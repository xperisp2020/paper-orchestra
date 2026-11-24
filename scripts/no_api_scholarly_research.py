#!/usr/bin/env python3
"""Keyless scholarly discovery and evidence drafting for the no-API workflow.

Uses public bibliographic endpoints only. It does not scrape Google Scholar or
claim that metadata-only records are full-text evidence.
"""
from __future__ import annotations

import argparse
import json
import re
import sys
import time
import urllib.error
import urllib.parse
import urllib.request
import xml.etree.ElementTree as ET
from collections import Counter
from pathlib import Path
from typing import Any

USER_AGENT = "PaperOrchestra/0.1 (https://github.com/xperisp2020/paper-orchestra)"
OPENALEX = "https://api.openalex.org/works"
CROSSREF = "https://api.crossref.org/works"
SEMANTIC_SCHOLAR = "https://api.semanticscholar.org/graph/v1/paper/search"
EUROPE_PMC = "https://www.ebi.ac.uk/europepmc/webservices/rest/search"
ARXIV = "https://export.arxiv.org/api/query"
DATACITE = "https://api.datacite.org/dois"
MAX_PER_SOURCE = 50


def request_json(url: str, timeout: int = 25) -> dict[str, Any]:
    req = urllib.request.Request(url, headers={"User-Agent": USER_AGENT, "Accept": "application/json"})
    with urllib.request.urlopen(req, timeout=timeout) as response:
        payload = json.loads(response.read().decode("utf-8"))
    if not isinstance(payload, dict):
        raise ValueError("The scholarly provider returned an unexpected response.")
    return payload


def clean_text(value: Any) -> str:
    if isinstance(value, list):
        value = " ".join(str(x) for x in value)
    value = re.sub(r"<[^>]+>", " ", str(value or ""))
    return re.sub(r"\s+", " ", value).strip()


def reconstruct_abstract(inverted: Any) -> str:
    if not isinstance(inverted, dict):
        return ""
    positions: dict[int, str] = {}
    for word, offsets in inverted.items():
        if isinstance(offsets, list):
            for offset in offsets:
                if isinstance(offset, int):
                    positions[offset] = str(word)
    return " ".join(positions[i] for i in sorted(positions))


def normalize_openalex(item: dict[str, Any]) -> dict[str, str]:
    doi = clean_text(item.get("doi")).lower()
    if doi.startswith("https://doi.org/"):
        doi = doi[16:]
    authors = []
    for authorship in item.get("authorships", []) or []:
        author = (authorship.get("author") or {}).get("display_name")
        if author:
            authors.append(clean_text(author))
    locations = item.get("locations") or []
    url = ""
    for location in locations:
        source = location.get("source") or {}
        candidate = clean_text(location.get("landing_page_url") or source.get("host_organization_lineage") or "")
        if candidate.startswith("http"):
            url = candidate
            break
    return {
        "title": clean_text(item.get("title")),
        "authors": "; ".join(authors[:8]),
        "year": str(item.get("publication_year") or ""),
        "doi": doi,
        "url": url or (("https://doi.org/" + doi) if doi else clean_text(item.get("id"))),
        "abstract": clean_text(reconstruct_abstract(item.get("abstract_inverted_index"))),
        "venue": clean_text((item.get("primary_location") or {}).get("source", {}).get("display_name")),
        "provider": "OpenAlex",
        "type": clean_text(item.get("type")),
        "is_oa": str(bool((item.get("open_access") or {}).get("is_oa"))).lower(),
    }


def normalize_crossref(item: dict[str, Any]) -> dict[str, str]:
    title = clean_text((item.get("title") or [""])[0])
    authors = []
    for author in item.get("author", []) or []:
        name = " ".join(x for x in [author.get("given"), author.get("family")] if x)
        if name:
            authors.append(clean_text(name))
    date_parts = ((item.get("published-print") or item.get("published-online") or item.get("created") or {}).get("date-parts") or [[""]])[0]
    year = str(date_parts[0]) if date_parts and date_parts[0] else ""
    doi = clean_text(item.get("DOI")).lower()
    return {
        "title": title,
        "authors": "; ".join(authors[:8]),
        "year": year,
        "doi": doi,
        "url": ("https://doi.org/" + doi) if doi else clean_text(item.get("URL")),
        "abstract": clean_text(item.get("abstract")),
        "venue": clean_text((item.get("container-title") or [""])[0]),
        "provider": "Crossref",
        "type": clean_text(item.get("type")),
        "is_oa": "unknown",
    }



def normalize_semantic_scholar(item: dict[str, Any]) -> dict[str, str]:
    authors = []
    for author in item.get("authors", []) or []:
        if isinstance(author, dict) and author.get("name"):
            authors.append(clean_text(author["name"]))
    external_ids = item.get("externalIds") or {}
    doi = clean_text(external_ids.get("DOI")).lower()
    return {
        "title": clean_text(item.get("title")),
        "authors": "; ".join(authors[:8]),
        "year": str(item.get("year") or ""),
        "doi": doi,
        "url": ("https://doi.org/" + doi) if doi else clean_text(item.get("url")),
        "abstract": clean_text(item.get("abstract")),
        "venue": clean_text(item.get("venue")),
        "provider": "Semantic Scholar",
        "type": "; ".join(item.get("publicationTypes") or []),
        "is_oa": str(bool(item.get("isOpenAccess"))).lower(),
    }



def request_xml(url: str, timeout: int = 25) -> str:
    req = urllib.request.Request(url, headers={"User-Agent": USER_AGENT, "Accept": "application/atom+xml, application/xml"})
    with urllib.request.urlopen(req, timeout=timeout) as response:
        return response.read().decode("utf-8")


def normalize_europe_pmc(item: dict[str, Any]) -> dict[str, str]:
    doi = clean_text(item.get("doi")).lower()
    urls = item.get("fullTextUrlList", {}).get("fullTextUrl", []) or []
    full_url = next((clean_text(x.get("url")) for x in urls if x.get("url")), "")
    return {"title": clean_text(item.get("title")), "authors": clean_text(item.get("authorString")),
            "year": clean_text(item.get("firstPublicationDate") or item.get("pubYear"))[:4],
            "doi": doi, "url": ("https://doi.org/" + doi) if doi else full_url,
            "abstract": clean_text(item.get("abstractText")), "venue": clean_text(item.get("journalTitle")),
            "provider": "Europe PMC", "type": clean_text(item.get("pubType")),
            "is_oa": str(item.get("isOpenAccess", "")).lower() or "unknown"}


def normalize_datacite(item: dict[str, Any]) -> dict[str, str]:
    attrs = item.get("attributes") or {}
    creators = attrs.get("creators") or []
    authors = [clean_text(x.get("name")) for x in creators if x.get("name")]
    titles = attrs.get("titles") or []
    descriptions = attrs.get("descriptions") or []
    abstract = next((clean_text(x.get("description")) for x in descriptions
                     if x.get("descriptionType") == "Abstract"), "")
    doi = clean_text(attrs.get("doi") or item.get("id")).lower()
    return {"title": clean_text(titles[0].get("title")) if titles else "",
            "authors": "; ".join(authors[:8]), "year": str(attrs.get("publicationYear") or ""),
            "doi": doi, "url": ("https://doi.org/" + doi) if doi else clean_text(attrs.get("url")),
            "abstract": abstract, "venue": clean_text(attrs.get("publisher")),
            "provider": "DataCite", "type": clean_text(attrs.get("resourceTypeGeneral")),
            "is_oa": "unknown"}


def normalize_arxiv_entry(entry: ET.Element) -> dict[str, str]:
    ns = {"a": "http://www.w3.org/2005/Atom"}
    get = lambda tag: clean_text(entry.findtext("a:" + tag, default="", namespaces=ns))
    authors = [clean_text(x.findtext("a:name", default="", namespaces=ns)) for x in entry.findall("a:author", ns)]
    published = get("published")
    return {"title": get("title"), "authors": "; ".join(x for x in authors if x)[:1000],
            "year": published[:4], "doi": "", "url": get("id"), "abstract": get("summary"),
            "venue": get("journal_ref") or "arXiv", "provider": "arXiv",
            "type": "preprint", "is_oa": "true"}


def discover(query: str, target: int = 20, sleeper=time.sleep) -> tuple[list[dict[str, str]], list[str]]:
    target = max(10, min(MAX_PER_SOURCE, target))
    rows = min(100, target * 2)
    crossref_query = urllib.parse.urlencode({
        "query.bibliographic": query, "rows": rows,
        "select": "DOI,title,author,published,published-print,published-online,created,container-title,URL,abstract,type",
        "sort": "score", "order": "desc",
    })
    semantic_query = urllib.parse.urlencode({
        "query": query, "limit": rows,
        "fields": "title,authors,year,abstract,venue,externalIds,url,publicationTypes,isOpenAccess",
    })
    providers = [
        ("OpenAlex", OPENALEX + "?" + urllib.parse.urlencode({"search": query, "per-page": rows, "sort": "relevance_score:desc"})),
        ("Crossref", CROSSREF + "?" + crossref_query),
        ("Semantic Scholar", SEMANTIC_SCHOLAR + "?" + semantic_query),
        ("Europe PMC", EUROPE_PMC + "?" + urllib.parse.urlencode({"query": query, "format": "json", "pageSize": rows, "resultType": "core"})),
        ("DataCite", DATACITE + "?" + urllib.parse.urlencode({"query": query, "page[size]": rows, "sort": "relevance"})),
        ("arXiv", ARXIV + "?" + urllib.parse.urlencode({"search_query": "all:" + query, "start": 0, "max_results": rows})),
    ]
    quota = max(2, (target + len(providers) - 1) // len(providers))
    sources: list[dict[str, str]] = []
    errors: list[str] = []
    seen: set[str] = set()
    for provider, url in providers:
        try:
            payload = {"_xml": request_xml(url)} if provider == "arXiv" else request_json(url)
            if provider == "OpenAlex":
                raw_rows, normalize = payload.get("results") or [], normalize_openalex
            elif provider == "Crossref":
                raw_rows, normalize = (payload.get("message") or {}).get("items") or [], normalize_crossref
            elif provider == "Semantic Scholar":
                raw_rows, normalize = payload.get("data") or [], normalize_semantic_scholar
            elif provider == "Europe PMC":
                raw_rows, normalize = (payload.get("resultList") or {}).get("result") or [], normalize_europe_pmc
            elif provider == "DataCite":
                raw_rows, normalize = payload.get("data") or [], normalize_datacite
            else:
                root = ET.fromstring(payload.get("_xml", ""))
                raw_rows, normalize = root.findall("{http://www.w3.org/2005/Atom}entry"), normalize_arxiv_entry
            added = 0
            for raw in raw_rows:
                record = normalize(raw)
                if len(record["title"]) < 8:
                    continue
                identity = record["doi"] or re.sub(r"\W+", "", record["title"].lower())
                if not identity or identity in seen:
                    continue
                seen.add(identity)
                sources.append(record)
                added += 1
                if added >= quota or len(sources) >= target:
                    break
        except (OSError, ValueError, urllib.error.URLError, TimeoutError, ET.ParseError, KeyError, TypeError) as exc:
            errors.append(f"{provider}: {type(exc).__name__}: {exc}")
        sleeper(0.25)
    return sources[:target], errors


def draft_sections(query: str, records: list[dict[str, str]], language: str) -> dict[str, str]:
    fa = language == "fa"
    citations = []
    abstracts = []
    for index, record in enumerate(records, 1):
        authors = record["authors"] or (("نویسندگان ثبت نشده" if fa else "Authors not listed"))
        year = record["year"] or (("بی‌تاریخ" if fa else "n.d."))
        venue = record["venue"] or (("محل انتشار ثبت نشده" if fa else "venue not listed"))
        doi = record["doi"]
        ref = f"DOI: https://doi.org/{doi}" if doi else record["url"]
        citations.append(f"[{index}] {authors} ({year}). {record['title']}. {venue}. {ref}")
        if record["abstract"]:
            abstracts.append(f"[{index}] {record['abstract'][:1800]}")

    refs = "\n".join(citations)
    evidence = "\n\n".join(abstracts)
    if fa:
        background = (
            f"موضوع جست‌وجو: «{query}».\n\n"
            f"این مرور مقدماتی بر پایهٔ فراداده‌های کتاب‌شناختی و چکیده‌های قابل‌دسترسی از "
            f"{len(records)} رکورد بازیابی‌شده از OpenAlex، Crossref، Semantic Scholar، Europe PMC، DataCite و arXiv تنظیم شده است. "
            "این مجموعه، جست‌وجوی نظام‌مندِ ثبت‌شده در پایگاه‌های اشتراکی مانند Scopus یا Web of Science "
            "نیست و به‌تنهایی جایگزین بررسی متن کامل مقالات نمی‌شود."
        )
        question = f"در ادبیات علمی نمایه‌شده، چه یافته‌ها، رویکردها و شکاف‌هایی دربارهٔ «{query}» گزارش شده‌اند؟"
        objectives = (
            "۱) شناسایی مطالعات مرتبط و توصیف ویژگی‌های کتاب‌شناختی آن‌ها.\n"
            "۲) استخراج موضوعات و روش‌های پرتکرار از چکیده‌های در دسترس.\n"
            "۳) تفکیک شواهد مستقیم از اطلاعاتی که صرفاً در فراداده آمده‌اند.\n"
            "۴) مشخص‌کردن خلأهای قابل‌آزمون برای پژوهش بعدی."
        )
        strategy = (
            "جست‌وجوی ماشینی با عبارت موضوعی عنوان پروژه در رابط عمومی OpenAlex Works و Crossref REST انجام شد. "
            "نتایج بر اساس ارتباط/ترتیب ارائه‌شده توسط هر سرویس دریافت و بر مبنای DOI یا عنوان یکتا شدند. "
            "Google Scholar در این نسخه به‌صورت خودکار خزش نمی‌شود؛ برای تکمیل پوشش، جست‌وجوی دستی همان عبارت در "
            "Google Scholar و ثبت نتایج واجد شرایط توصیه می‌شود."
        )
        inclusion = (
            "عنوان دارای ارتباط موضوعی روشن با عبارت جست‌وجو؛ رکورد دارای عنوان قابل‌شناسایی؛ "
            "ترجیح رکوردهای دارای DOI، سال انتشار و نام نشریه/ناشر. این معیارها غربالگری اولیه‌اند، نه ارزیابی کیفیت."
        )
        exclusion = (
            "رکوردهای فاقد عنوان معتبر، موارد تکراری بر اساس DOI/عنوان، و مواردی که ارتباط موضوعی آن‌ها از عنوان "
            "قابل دفاع نیست. به علت محدودیت دسترسی به متن کامل، حذف بر مبنای کیفیت روش‌شناختی هنوز انجام نشده است."
        )
        synthesis = (
            f"تعداد رکوردهای یکتای بازیابی‌شده: {len(records)}.\n"
            f"چکیده‌های قابل‌دسترسی: {len(abstracts)}.\n\n"
            "متن چکیده‌های قابل‌دسترسی برای تحلیل محتوای مقدماتی در زیر آورده شده است. "
            "تا زمانی که متن کامل و روش هر مطالعه بررسی نشده، از این داده‌ها نتیجهٔ علّی، اندازه اثر، "
            "یا ادعای اجماع علمی استنتاج نمی‌شود.\n\n" + evidence + "\n\nمنابع کتاب‌شناختی:\n" + refs
        )
        findings = (
            "یافتهٔ قابل اتکا در این مرحله، فهرست مطالعات بازیابی‌شده و اطلاعات کتاب‌شناختی آن‌هاست. "
            "جمع‌بندی ماهوی باید پس از بررسی متن کامل، استخراج نمونه/روش/نتایج هر مطالعه و ارزیابی خطر سوگیری تکمیل شود.\n\n"
            "رکوردها و چکیده‌های قابل دسترس:\n" + evidence + "\n\nمنابع کتاب‌شناختی:\n" + refs
        )
        methodology = (
            "طرح پیشنهادی: مرور دامنه‌ای مقدماتی با پروتکل قابل تکرار. پایگاه‌ها: OpenAlex و Crossref؛ "
            "Google Scholar فقط از طریق جست‌وجوی دستی و مطابق شرایط استفاده. ثبت تاریخ جست‌وجو، عبارت‌ها، "
            "تعداد نتایج، DOI، معیارهای ورود/خروج و دلایل حذف ضروری است. پس از حذف موارد تکراری، "
            "دو مرحله غربالگری عنوان/چکیده و متن کامل، سپس استخراج داده و ارزیابی کیفیت انجام شود."
        )
        limitations = (
            "این اجرا به APIهای فراداده عمومی متکی است؛ همه رکوردها چکیده یا متن کامل ندارند. "
            "ترتیب رتبه‌بندی سرویس‌ها معادل ارزیابی کیفیت نیست. پوشش Google Scholar در این اجرا خودکار نیست. "
            "غربالگری مستقل دو ارزیاب، ارزیابی خطر سوگیری و راستی‌آزمایی دستی همه DOIها هنوز لازم است."
        )
        future = (
            "۱) اجرای جست‌وجوی دستی تکمیلی در Google Scholar و پایگاه‌های تخصصی در دسترس دانشگاه.\n"
            "۲) دریافت و غربالگری متن کامل مطالعات واجد شرایط.\n"
            "۳) استخراج ساختاریافته هدف، طرح، نمونه، ابزار، نتایج و محدودیت‌ها.\n"
            "۴) ارزیابی کیفیت و به‌روزرسانی سنتز شواهد."
        )
        sources_text = refs
    else:
        background = (
            f"Search topic: {query}.\n\nThis preliminary review uses bibliographic metadata and accessible abstracts "
            f"from {len(records)} records retrieved from OpenAlex, Crossref, Semantic Scholar, Europe PMC, DataCite, and arXiv. It is not a registered systematic "
            "search across subscription databases and does not replace full-text appraisal."
        )
        question = f"What findings, methods, and evidence gaps are reported in indexed literature about {query}?"
        objectives = "1. Identify relevant indexed studies.\n2. Summarize recurring topics and methods in available abstracts.\n3. Separate direct evidence from metadata-only information.\n4. Identify testable research gaps."
        strategy = "Automated topic search was run against the public OpenAlex Works and Crossref REST endpoints. Records were deduplicated by DOI or normalized title. Google Scholar is not scraped automatically; a manual supplementary search is recommended."
        inclusion = "Clearly topic-relevant titles with identifiable bibliographic records; DOI, publication year, and venue are preferred. This is preliminary screening, not quality appraisal."
        exclusion = "Records without valid titles, duplicates, and records whose relevance cannot be defended from their metadata. Full-text quality exclusions have not yet been performed."
        synthesis = f"Unique records retrieved: {len(records)}.\nAbstracts available: {len(abstracts)}.\n\nAbstracts for preliminary content analysis:\n{evidence}"
        findings = "At this stage, the defensible findings are the retrieved records and their bibliographic metadata. Substantive conclusions require full-text review and extraction of each study's design, sample, methods, and results.\n\n" + evidence
        methodology = "Proposed design: a preliminary scoping review. Search OpenAlex and Crossref; supplement with manual Google Scholar and subject-database searches where licensed. Record dates, queries, result counts, DOI, inclusion/exclusion decisions, and reasons. Deduplicate, screen titles/abstracts and full texts, extract data, and appraise quality."
        limitations = "Public metadata APIs do not provide abstracts/full text for every record. Provider ranking is not quality appraisal. Google Scholar was not automatically crawled. Independent screening, risk-of-bias appraisal, and manual DOI verification remain outstanding."
        future = "1. Supplement with manual Google Scholar and institutionally available databases.\n2. Retrieve and screen full texts.\n3. Extract design, sample, instruments, results, and limitations.\n4. Appraise quality and update the synthesis."
        sources_text = refs

    return {
        "research_question": question,
        "research_background": background,
        "research_objectives": objectives,
        "research_methodology": methodology,
        "search_strategy": strategy,
        "inclusion_criteria": inclusion,
        "exclusion_criteria": exclusion,
        "evidence_synthesis": synthesis,
        "findings": findings,
        "limitations": limitations,
        "future_work": future,
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--query", required=True)
    parser.add_argument("--language", choices=("fa", "en"), default="fa")
    parser.add_argument("--target", type=int, default=20)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    records, errors = discover(args.query, args.target)
    if len(records) < 10:
        print(json.dumps({"ok": False, "count": len(records), "errors": errors,
                          "message": "Fewer than 10 unique scholarly records were retrieved; refusing to fabricate a bibliography."},
                         ensure_ascii=False))
        return 2
    result = {
        "ok": True,
        "query": args.query,
        "count": len(records),
        "target": max(10, min(MAX_PER_SOURCE, args.target)),
        "providers": sorted({r["provider"] for r in records}),
        "records": records,
        "sections": draft_sections(args.query, records, args.language),
        "warnings": errors + [
            "Metadata and abstracts are not full-text evidence.",
            "Google Scholar is not automatically scraped; perform a manual supplementary search.",
            "The generated synthesis is preliminary and does not claim independent peer review or risk-of-bias assessment."
        ],
    }
    destination = Path(args.output)
    destination.parent.mkdir(parents=True, exist_ok=True)
    temporary = destination.with_suffix(destination.suffix + ".tmp")
    temporary.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding="utf-8")
    temporary.replace(destination)
    print(json.dumps({"ok": True, "count": len(records), "providers": result["providers"]}, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
