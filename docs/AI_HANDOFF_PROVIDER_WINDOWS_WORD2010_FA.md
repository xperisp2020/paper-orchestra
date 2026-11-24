# برگهٔ مأموریت مستقل برای هوش مصنوعی دوم
## تکمیل آزمون‌های زندهٔ Provider و اعتبارسنجی واقعی Windows/Word 2010

### مقصد قطعی و محدودیت‌ها
- این سند مأموریت مسیر B در تقسیم موازی است. مسیر A هم‌زمان روی ادغام Crossref کار می‌کند؛ برای جلوگیری از تعارض، فایل‌های اصلی مسیر A را تغییر نده، مگر باگ مسدودکننده‌ای را با شواهد ثابت کرده و ابتدا گزارش کنی.
- نقشهٔ کل تقسیم: `docs/PARALLEL_REMAINING_WORK_SPLIT_FA.md`.
- فقط مخزن: https://github.com/xperisp2020/paper-orchestra
- فقط شاخه: `work/yadavarifork`
- مالک مخزن: `xperisp2020`؛ شناسهٔ مخزن: `1467327081`.
- قبل از هر تغییر، owner/repo id، branch و HEAD را دوباره بررسی کن. هیچ فورک یا مخزن دیگری را تغییر نده.
- اسرار را در چت، فایل، commit، log یا خروجی CI ننویس. از GitHub Actions Secrets یا متغیرهای محیطی امن استفاده کن.
- هیچ انتشار عمومی، هزینه‌کرد یا دسترسی جدیدی بدون اجازهٔ صریح انجام نده.
- هر تغییر، خطا، نتیجه، artifact، hash و TODO را در انتهای فایل `yadavarifork` ثبت کن.

### وضعیت مبنا
گیت‌های خودکار قبلی موفق‌اند:
- Python Safety run: https://github.com/xperisp2020/paper-orchestra/actions/runs/45347795586 (۷۰ تست در Ubuntu و ۷۰ تست در Windows).
- Research Runtime: https://github.com/xperisp2020/paper-orchestra/actions/runs/45347795580 (Python 3.12 روی Windows و LaTeX/BibTeX واقعی روی Ubuntu).
- Windows packaging: https://github.com/xperisp2020/paper-orchestra/actions/runs/45347795650 (ساخت Portable/Installer، استخراج و smoke test، نصب بی‌صدا، اجرای اپ، uninstall و حفظ دادهٔ کاربر روی clean runner).
- آخرین artifactهای تأییدشده از run 45356339161: Portable id 14903375441 با SHA-256 archive digest برابر bd96c20e79558029fbbfe25841e6a3f54797eff50020eb5df63c8735b95d5646؛ Installer id 14902098063 با SHA-256 archive digest برابر f86742f3136f3344422fcda05a5798789bec3209fa94f4b532c6a41ada36bca5. اجرای clean-runner هر دو را با موفقیت استخراج/اجرا کرد، نصب و uninstall را انجام داد و حفظ دادهٔ sentinel را تأیید کرد. این digestها hash فایل ZIP artifact هستند، نه لزوماً hash باینری داخل ZIP.
- این نتایج CI هستند؛ Portable و Installer روی Windows Server 2022 clean runner به‌همراه مسیر فاصله‌دار/فارسی، smoke export، نصب، uninstall و حفظ داده تست شده‌اند؛ اما تست روی رایانهٔ واقعی کاربر و Word 2010 نیستند.

### مأموریت A — آزمون Providerهای زنده (فقط با credential مجاز)
1. کد routing را مرور کن: `utils/llm_backend_utils.py`، `utils/openai_utils.py`، `utils/gemini_utils.py`، `app_cpp/src/main.cpp` و CLI.
2. با secrets محیط اجرا، یک smoke test حداقلی و کم‌هزینه برای هر مسیر مجاز اجرا کن:
   - Gemini با `GEMINI_API_KEY` یا نام متغیر رسمی مورد استفادهٔ کد.
   - OpenAI با `OPENAI_API_KEY`.
   - OpenRouter Free با `OPENROUTER_API_KEY` و مدل رایگان معتبر از allowlist رابط.
3. هیچ‌گاه کلید را echo، print یا در exception/log ذخیره نکن. خروجی تست فقط provider، model، HTTP/status، زمان و موفق/ناموفق باشد.
4. تست 429، timeout، پاسخ خالی، لغو اجرا و redaction را با mock جداگانه بررسی کن؛ برای 429 حلقهٔ retry بی‌پایان نساز.
5. اگر کلیدی در دسترس نیست، آن provider را BLOCKED اعلام کن؛ کلید را از کاربر در متن چت نخواه.
6. نتیجهٔ زنده را از آزمون mock جدا گزارش کن. صرف عبور mock به معنی اتصال واقعی نیست.

### مأموریت B — آزمون واقعی Windows و Word 2010
1. روی یک Windows واقعی/VM پاک، نسخهٔ Portable و Installer artifactهای بالا را دریافت و SHA-256 را با مقادیر ثبت‌شده مقایسه کن.
2. Portable را از مسیر دارای فاصله و مسیر غیرلاتین اجرا کن؛ شروع برنامه، ساخت پروژه، ذخیره، بستن و بازگشایی را بررسی کن.
3. Installer را نصب کن، برنامه را اجرا کن، یک پروژهٔ آزمایشی بساز، uninstall کن و حفظ/حذف داده‌های کاربر را مطابق طراحی بررسی کن.
4. اگر Word 2010 نصب است، DOCX خروجی را با Word 2010 باز کن؛ صفحه‌بندی، فونت فارسی، راست‌به‌چپ، جدول‌ها، شکست صفحه، سربرگ/پاورقی و چاپ/PDF را بررسی کن. اگر Word 2010 در دسترس نیست، صریحاً BLOCKED بزن؛ LibreOffice جایگزین معادل Word 2010 نیست.
5. هیچ دادهٔ واقعی دانشجو/استاد وارد نکن؛ از دادهٔ ساختگی استفاده کن و قسمت‌های اداری را خالی نگه دار.
6. برای هر خطا، نسخهٔ Windows، معماری، نسخهٔ Word، مسیر اجرا، گام بازتولید، log پاک‌سازی‌شده و screenshot بدون دادهٔ حساس را ثبت کن.
7. اصلاح را فقط در همین مخزن/شاخه انجام بده، سپس build و clean-runner CI را دوباره اجرا کن. انتشار عمومی یا انتشار نسخهٔ نصبی جدید بدون تأیید کاربر ممنوع است.

### معیار پذیرش و تحویل
- ماتریس وضعیت Providerها: LIVE PASS / MOCK ONLY / BLOCKED.
- ماتریس Windows: Portable، Installer، مسیر غیرلاتین، ذخیره/بازگشایی، uninstall/data preservation.
- وضعیت دقیق Word 2010: PASS فقط اگر واقعاً روی Word 2010 اجرا شده باشد.
- هیچ secret در artifacts/logs/commits نباشد.
- لینک runها، artifactها، SHA-256، commitها و موارد باقی‌مانده را ثبت کن.
- دفتر `yadavarifork` را append کن؛ سابقهٔ قبلی را پاک یا بازنویسی نکن.
- در پایان یک گزارش کوتاه با موارد PASS/BLOCKED و شواهد بده. ادعای تستی که انجام نشده ممنوع است.

### اولویت اجرا
ابتدا providerهای قابل دسترس و بدون هزینه/با کمترین هزینه را smoke-test کن؛ سپس Windows/Word. اگر دستگاه یا Word 2010 در دسترس نیست، تست را متوقف و BLOCKED ثبت کن و به‌جای حدس زدن، مراحل دقیق اجرای دستی را تحویل بده.


### آخرین چرخهٔ عیب‌یابی بسته‌بندی (2026-11-24)
- نخستین clean-runner run 45356257643 در مرحلهٔ نصب Installer شکست خورد، چون آرگومان /DIR برای مسیر دارای فاصله/فارسی بدون نقل‌قول کامل ارسال شده بود؛ Portable smoke و export همان run پاس شده بود.
- اصلاح در commit f1d5cb39d90afb645d3820ab83c887eb5cbbcba2 نقل‌قول کامل مسیر نصب را در PowerShell اعمال کرد.
- run بعدی 45356339161 با conclusion=success تمام شد: ساخت و smoke برنامه، ساخت هر دو artifact، clean-runner Portable smoke/export، نصب silent، اجرای نصب‌شده، uninstall و حفظ داده sentinel همگی پاس شدند.
- این نتیجهٔ CI روی Windows Server 2022 است و جای تست فیزیکی user device/Word 2010 را نمی‌گیرد.
