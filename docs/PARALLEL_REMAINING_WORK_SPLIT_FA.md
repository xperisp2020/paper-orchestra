# تقسیم رسمی کارهای باقی‌مانده به دو مسیر موازی
تاریخ: 2026-11-24

## مسیر A — دستیار اصلی: اتصال Crossref به pipeline
**هدف:** تکمیل یک قابلیت کدنویسیِ مستقل و آزمون‌پذیر.
- اتصال opt-in helper موجود Crossref به Literature Agent و هر دو writer.
- افزودن گزینهٔ CLI، ثبت status در citation_map، حفظ DOI و جلوگیری از شکست pipeline هنگام outage.
- افزودن تست‌های regression و اجرای CI در Ubuntu و Windows و Research Runtime.
- شرح مسیر: `docs/WORKSTREAM_A_CROSSREF_PIPELINE_FA.md`.

## مسیر B — هوش مصنوعی دوم: اعتبارسنجی زنده و محیط واقعی
**هدف:** پیشبرد موازی مواردی که به credentials و محیط Windows/Word واقعی نیاز دارند.
- smoke test زنده و کم‌هزینه برای Gemini، OpenAI و OpenRouter Free با secrets امن؛ تفکیک LIVE PASS از MOCK ONLY و BLOCKED.
- آزمون timeout/429/cancellation/redaction با mock، بدون retry بی‌نهایت و بدون چاپ secrets.
- اعتبارسنجی SHA artifactهای Windows؛ آزمون Portable و Installer روی Windows واقعی/VM، مسیر غیرلاتین و دارای فاصله، شروع/ذخیره/بازگشایی، نصب/uninstall و حفاظت داده.
- آزمون DOCX در Microsoft Word 2010 واقعی: RTL فارسی، فونت، صفحه‌بندی، جدول، page break، header/footer، چاپ/PDF.
- نبود credential/device/Word را BLOCKED گزارش کند؛ تست انجام‌نشده را PASS اعلام نکند.
- شرح اجرایی و معیار پذیرش: `docs/AI_HANDOFF_PROVIDER_WINDOWS_WORD2010_FA.md`.

## مرز همکاری
- فقط مخزن `xperisp2020/paper-orchestra`، repo id `1467327081` و شاخهٔ `work/yadavarifork`.
- پیش از هر write، owner/repo id، branch، HEAD و SHA فایل مقصد دوباره بررسی شود.
- مسیر B فایل‌های اصلی مسیر A را در خلال کار تغییر ندهد؛ در صورت نیاز، تعارض و diff را ثبت و هماهنگ کند.
- هیچ secret، داده واقعی دانشجو/استاد، یا اطلاعات حساس در commit/log/artifact نرود.
- همه نتایج، خطاها، اصلاحات، SHA/run/artifact در انتهای `yadavarifork` ثبت شوند؛ سابقه پاک یا بازنویسی نشود.
- انتشار عمومی، release یا هزینه‌کرد بدون اجازه صریح ممنوع است.

## تحویل نهایی
مسیر A باید commit و CI قابل اثبات ارائه کند؛ مسیر B باید ماتریس provider و Windows/Word را با شواهد و موارد BLOCKED تحویل دهد. سپس دو نتیجه در ممیزی نهایی ادغام می‌شوند.


## وضعیت به‌روزشده پس از ممیزی (2026-11-24)
- مسیر A (اتصال Crossref) پیاده‌سازی شده و Python Safety، Research Runtime/LaTeX و Windows clean-runner verification شواهد PASS دارند.
- کارهای خودکار قبلی در این نقطه نباید بدون regression تکرار شوند.
- مسیر B اکنون بر ارائهٔ قابل ممیزی تغییرات قبلی AI دوم، providerهای واقعی، Windows واقعی/VM و Microsoft Word 2010 واقعی متمرکز است.
- در ممیزی GitHub با HEAD `743d19e11805e4e33f02c8133acde826187a0a31` commit تازه/PR جدید قابل انتساب به AI دوم پیدا نشد؛ ممکن است تغییرات در محیط محلی و pushنشده باشند.
- برای ادامه، از `docs/AI_HANDOFF_PROVIDER_WORD2010_V2_FA.md` و `docs/AI_SECOND_PASS_RESULT_TEMPLATE_FA.md` استفاده شود.
- گزارش ممیزی: `docs/AI_SECOND_PASS_AUDIT_FA.md`.
