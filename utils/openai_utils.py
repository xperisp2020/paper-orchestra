# Copyright 2026 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from typing import Any
import re
import json
import time
import os
from utils.security_utils import redact_secrets
import sys
import threading
import cv2  # type: ignore
import base64
import mimetypes

from openai import OpenAI  # type: ignore
from dotenv import load_dotenv  # type: ignore

dot_file = os.path.join(os.path.dirname(__file__), "../.env")
load_dotenv(dot_file)

_openai_clients = {}
_openai_client_lock = threading.Lock()
_openrouter_rate_lock = threading.Lock()
_openrouter_last_request = 0.0
_OPENROUTER_MIN_INTERVAL_SECONDS = 3.05


def _is_openrouter_model(model_name: str) -> bool:
    normalized = (model_name or "").strip().lower()
    return "/" in normalized or normalized.startswith("openrouter/")


def get_openai_client(model_name: str = None) -> OpenAI:
    """Return a cached OpenAI or OpenRouter-compatible client.

    OpenRouter uses the OpenAI SDK with its own base URL and API key. The
    provider is selected from the model ID, so OpenAI and OpenRouter keys can
    coexist without being mixed.
    """
    provider = "openrouter" if _is_openrouter_model(model_name) else "openai"
    with _openai_client_lock:
        if provider in _openai_clients:
            return _openai_clients[provider]

        if provider == "openrouter":
            api_key = os.environ.get("OPENROUTER_API_KEY")
            if not api_key:
                raise ValueError("OPENROUTER_API_KEY must be set to use OpenRouter models.")
            client = OpenAI(
                api_key=api_key,
                base_url="https://openrouter.ai/api/v1",
                max_retries=0,
            )
        else:
            api_key = os.environ.get("OPENAI_API_KEY")
            if not api_key:
                raise ValueError("OPENAI_API_KEY must be set to use OpenAI models.")
            client = OpenAI(api_key=api_key)

        _openai_clients[provider] = client
        return client


def _wait_for_openrouter_rate_slot():
    """Serialize free-router calls to stay below OpenRouter's 20 RPM cap."""
    global _openrouter_last_request
    with _openrouter_rate_lock:
        now = time.monotonic()
        wait_seconds = _OPENROUTER_MIN_INTERVAL_SECONDS - (now - _openrouter_last_request)
        if wait_seconds > 0:
            time.sleep(wait_seconds)
        _openrouter_last_request = time.monotonic()


def parse_openai_json_results(response: str):
    if not response or not isinstance(response, str):
        return None

    json_pattern = r"```json(.*?)```"
    matches = re.findall(json_pattern, response, re.DOTALL)

    if not matches:
        json_pattern = r"(\{.*\}|\[.*\])"
        matches = re.findall(json_pattern, response, re.DOTALL)

    for json_string in matches:
        json_string = json_string.strip()
        try:
            parsed_json = json.loads(json_string)
            return parsed_json
        except json.JSONDecodeError:
            try:
                json_string_clean = re.sub(r"[\x00-\x1F\x7F]", "", json_string)
                json_string_clean = re.sub(r",\s*([\]}])", r"\1", json_string_clean)
                json_string_clean = re.sub(
                    r'\\(?![\\"/bfnrtu])', r"\\\\", json_string_clean
                )

                parsed_json = json.loads(json_string_clean)
                return parsed_json
            except json.JSONDecodeError:
                continue

    try:
        start_idx = response.find("{")
        end_idx = response.rfind("}")
        if start_idx != -1 and end_idx != -1 and end_idx > start_idx:
            json_string = response[start_idx : end_idx + 1]
            json_string_clean = re.sub(r"[\x00-\x1F\x7F]", "", json_string)
            json_string_clean = re.sub(r",\s*([\]}])", r"\1", json_string_clean)
            json_string_clean = re.sub(
                r'\\(?![\\"/bfnrtu])', r"\\\\", json_string_clean
            )
            return json.loads(json_string_clean)
    except Exception:
        pass

    try:
        start_idx = response.find("[")
        end_idx = response.rfind("]")
        if start_idx != -1 and end_idx != -1 and end_idx > start_idx:
            json_string = response[start_idx : end_idx + 1]
            json_string_clean = re.sub(r"[\x00-\x1F\x7F]", "", json_string)
            json_string_clean = re.sub(r",\s*([\]}])", r"\1", json_string_clean)
            json_string_clean = re.sub(
                r'\\(?![\\"/bfnrtu])', r"\\\\", json_string_clean
            )
            return json.loads(json_string_clean)
    except Exception:
        pass

    return None  # No valid JSON found


def call_openai_models_with_content(
    content: list[Any],
    model_name: str,
    max_retries: int = 5,
    base_interval_sec: int = 5,
    result_parsing_func: callable = parse_openai_json_results,
    generation_configs: dict = {},
    check_parsed_response_not_none: bool = True,
    system_prompt: str = None,
):
    raw_response = None
    parsed_response = None
    is_openrouter = _is_openrouter_model(model_name)
    effective_max_retries = 1 if is_openrouter else max_retries
    openai_client = get_openai_client(model_name)

    for attempt in range(effective_max_retries):
        try:
            if is_openrouter:
                _wait_for_openrouter_rate_slot()
            messages = []
            if system_prompt:
                role = "developer" if model_name.startswith(("o1", "o3", "o4")) else "system"
                messages.append({"role": role, "content": system_prompt})

            messages.append({"role": "user", "content": content})

            response = openai_client.chat.completions.create(
                model=model_name,
                messages=messages,
                **generation_configs,
            )
            raw_response = response.choices[0].message.content
            parsed_response = result_parsing_func(raw_response)

            if check_parsed_response_not_none:
                assert (
                    parsed_response is not None
                ), f"Parsed response is None, Here is the raw response:\n{raw_response}"

            return {
                "raw_response": raw_response,
                "parsed_response": parsed_response,
            }

        except Exception as e:
            print(
                f"Warning: OpenAI-compatible API call (model={model_name}) failed on attempt {attempt + 1} of {effective_max_retries}. Error: {redact_secrets(e)}",
            )

            # Failed OpenRouter free-tier attempts count toward the provider quota;
            # do not burn the daily allowance by retrying a rate-limit response.
            if is_openrouter and getattr(e, "status_code", None) == 429:
                print("OpenRouter rate limit reached; stopping retries.")
                raise

            if attempt + 1 == effective_max_retries:
                print(f"Error: All {effective_max_retries} attempt(s) failed.")
                raise e

            delay = base_interval_sec * (2 ** (attempt - 1)) if attempt > 0 else 0
            print(f"Retrying in {delay} seconds...")
            if delay > 0:
                time.sleep(delay)

    print("Error: Exited retry loop unexpectedly.")
    return {
        "raw_response": raw_response,
        "parsed_response": parsed_response,
    }


def call_openai_models_with_text_prompt(
    prompt: str,
    model_name: str,
    result_parsing_func: callable = parse_openai_json_results,
    generation_configs: dict = {},
    check_parsed_response_not_none: bool = True,
    max_retries: int = 5,
    base_interval_sec: int = 5,
    system_prompt: str = None,
):
    return call_openai_models_with_content(
        content=[{"type": "text", "text": prompt}],
        model_name=model_name,
        result_parsing_func=result_parsing_func,
        generation_configs=generation_configs,
        check_parsed_response_not_none=check_parsed_response_not_none,
        max_retries=max_retries,
        base_interval_sec=base_interval_sec,
        system_prompt=system_prompt,
    )


def get_openai_image_part_from_path(image_path: str):
    with open(image_path, "rb") as image_file:
        base64_image = base64.b64encode(image_file.read()).decode("utf-8")

    mime_type, _ = mimetypes.guess_type(image_path)
    if not mime_type or not mime_type.startswith("image/"):
        mime_type = "image/jpeg"

    image_content = [
        {
            "type": "image_url",
            "image_url": {
                "url": f"data:{mime_type};base64,{base64_image}",
            },
        }
    ]
    return image_content
