# قالب گزارش تحویل هوش مصنوعی دوم — مرحلهٔ دوم

## A) هویت مخزن و تغییرات قبلی
- repository full name:
- repository id:
- branch:
- base HEAD قبل از تغییر:
- current HEAD:
- آیا تغییرات قبلی push شده‌اند؟:
- commit SHA و URL:
- فایل‌های تغییرکرده:
- خلاصهٔ diff:
- مواردی که فقط local/commitنشده مانده‌اند:
- آیا تغییر با مسیر A تداخل دارد؟:
- نتیجهٔ review تغییرات قبلی: PASS / NEEDS FIX / NOT PROVIDED

## B) Providerهای زنده

| Provider | Model | Credential available? | Live request? | HTTP/status | Duration | Result |
|---|---|---|---|---|---|---|
| Gemini | | | | | | LIVE PASS / LIVE FAIL / BLOCKED |
| OpenAI | | | | | | LIVE PASS / LIVE FAIL / BLOCKED |
| OpenRouter Free | | | | | | LIVE PASS / LIVE FAIL / BLOCKED |

- Mock tests اجراشده:
- 429/timeout/cancel/redaction:
- URL اجرای CI:
- اطمینان از حذف secrets از logs/artifacts:

## C) Windows واقعی/VM

| آزمون | محیط و نسخه | نتیجه | شواهد |
|---|---|---|---|
| hash Portable ZIP | | | |
| Portable launch | | | |
| مسیر دارای فاصله و حروف فارسی | | | |
| ساخت/ذخیره/بازگشایی پروژه | | | |
| Installer silent/custom path | | | |
| اجرای نسخهٔ نصب‌شده | | | |
| uninstall | | | |
| حفظ دادهٔ کاربر | | | |

## D) Microsoft Word 2010

- Word version/build:
- 32-bit یا 64-bit:
- Windows version:
- DOCX file hash:
- RTL فارسی:
- فونت و fallback:
- جدول‌ها:
- page breaks و صفحه‌بندی:
- header/footer:
- print/PDF:
- باز/ذخیره مجدد:
- screenshot/log بدون اطلاعات شخصی:
- نتیجه: PASS / FAIL / BLOCKED

## E) Crossref live
- آیا درخواست واقعی اجرا شد؟:
- DOI آزمایش‌شده (عمومی):
- زمان/HTTP status:
- نتیجه:
- محدودیت‌ها:

## F) مشکلات و اصلاحات
برای هر مورد:
- شرح:
- severity:
- مراحل بازتولید:
- علت ریشه‌ای:
- اصلاح:
- commit SHA:
- تست پس از اصلاح:
- نتیجه:

## G) وضعیت نهایی
- موارد PASS:
- موارد FAIL:
- موارد BLOCKED و علت دقیق:
- TODOهای بعدی:
- آخرین HEAD:
- لینک دفتر `yadavarifork`:
- تأیید عدم افشای secrets: بله/خیر

**قاعده:** تست mock به‌جای live پذیرفته نیست؛ اجرای VM به‌جای دستگاه واقعی گزارش نشود؛ LibreOffice به‌جای Word 2010 پذیرفته نیست.


## H) موجودی دقیق workspace و تحویل patch
- workspace قبلی در دسترس بود؟ بله/خیر
- remote و branch:
- base HEAD:
- workspace HEAD:
- `git status --short`:
- `git diff --stat`:
- `git diff --cached --stat`:
- untracked files:
- modified/added/deleted/renamed counts:
- commit SHAها:
- patch/bundle path or URL:
- patch SHA-256:
- secrets/PII review:
- وضعیت: PATCH READY / ALREADY PRESENT / NO LOCAL WORKSPACE ACCESS / NO PRIOR CHANGES
