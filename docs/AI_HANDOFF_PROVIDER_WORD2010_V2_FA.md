# مأموریت مرحلهٔ دوم هوش مصنوعی — Provider زنده، Word 2010 و تحویل تغییرات قبلی
نسخه: 2.0
تاریخ مبنا: 2026-11-24

## 0) مهم: ابتدا تغییرات قبلی خودت را آشکار و قابل ممیزی کن
کاربر گفته است که این مأموریت را با نسخهٔ قبلی بررسی کرده‌ای و تغییراتی انجام داده‌ای. اما در ممیزی GitHub مورخ 2026-11-24 هیچ commit تازه، branch تازه، Pull Request یا فایل تحویلی از آن تغییرات مشاهده نشد. HEAD ثبت‌شده در شروع ممیزی:
`743d19e11805e4e33f02c8133acde826187a0a31`.

**پیش از هر تغییر جدید:**
1. در workspace خودت نام repository، remote، branch، HEAD و working tree را نشان بده.
2. گزارش بده دقیقاً چه فایل‌هایی را تغییر داده‌ای؛ `git status --short`، `git diff --stat` و diff کامل یا patch پاک‌سازی‌شده را بررسی کن.
3. commitهای ساخته‌شده را با SHA، پیام و تاریخ فهرست کن.
4. اگر تغییرات را commit/push کرده‌ای، URL commit یا workflow run و SHA آن را ارائه کن.
5. اگر تغییرات فقط محلی و commitنشده‌اند، ابتدا patch استانداردی بساز و محتوا/فهرست فایل‌ها را گزارش کن؛ آن‌ها را بدون بررسی به HEAD فعلی تحمیل نکن.
6. اگر تغییراتی در مخزن دیگری یا شاخه‌ای غیر از مقصد مجاز انجام شده، آن‌ها را منتقل یا push نکن؛ فقط patch را برای ممیزی ارائه کن.
7. اگر واقعاً هیچ تغییر قبلی نداری، صریحاً بنویس «تغییر قبلی قابل ارائه ندارم» و به مأموریت‌های زیر برو.

قالب گزارش: `docs/AI_SECOND_PASS_RESULT_TEMPLATE_FA.md`.

## 1) مقصد و محدودیت غیرقابل مذاکره
- فقط مخزن: https://github.com/xperisp2020/paper-orchestra
- repository id: `1467327081`
- owner: `xperisp2020`
- فقط branch: `work/yadavarifork`
- هیچ fork/repository دیگری را تغییر نده.
- پیش از هر write، owner/repo id، branch، HEAD و SHA فایل مقصد را دوباره بخوان.
- از force-push، reset مخرب، حذف تاریخچه و overwrite تغییرات دیگران استفاده نکن.
- همهٔ نتایج/تغییرات/خطاها به‌صورت append-only در دفتر `yadavarifork` ثبت شوند.
- هیچ API key، token، password، .env، دادهٔ شخصی دانشجو/استاد یا اطلاعات محرمانه در چت، commit، artifact یا log وارد نشود.
- برای دسترسی به providerها فقط از GitHub Actions Secrets یا environment امنی که از قبل مجاز است استفاده کن؛ کلید را در چت درخواست نکن.
- انتشار عمومی، release، نصب سرویس جدید با هزینه، یا هر هزینه‌کرد بدون اجازهٔ صریح کاربر ممنوع است.

## 2) وضعیت پایه‌ای که نباید بی‌دلیل دوباره انجام شود
تمام موارد زیر شواهد PASS دارند:
- Python Safety: run `45356267071`، Ubuntu و Windows؛ هر کدام 86 تست.
  https://github.com/xperisp2020/paper-orchestra/actions/runs/45356267071
- Research Runtime: run `45356069474`، Windows Python 3.12 bootstrap موفق و Ubuntu واقعی TeX Live 2025/BibTeX/PDF bibliography-backed موفق.
  https://github.com/xperisp2020/paper-orchestra/actions/runs/45356069474
- Windows Build و clean-runner QA: run `45356339161`؛ CMake configure/build، app smoke، ساخت Portable/Installer، اجرای Portable از مسیر فارسی و دارای فاصله، تولید PDF/DOCX، نصب silent، اجرای نسخهٔ نصب‌شده، uninstall و حفظ دادهٔ sentinel موفق.
  https://github.com/xperisp2020/paper-orchestra/actions/runs/45356339161
- Portable artifact id `14903375441`؛ SHA-256 archive `bd96c20e79558029fbbfe25841e6a3f54797eff50020eb5df63c8735b95d5646`.
- Installer artifact id `14902098063`؛ SHA-256 archive `f86742f3136f3344422fcda05a5798789bec3209fa94f4b532c6a41ada36bca5`.
- انقضای artifactها: `2027-02-22T08:27:57Z`.
- لینک دانلود Portable: https://api.github.com/repos/xperisp2020/paper-orchestra/actions/artifacts/14903375441/zip
- لینک دانلود Installer: https://api.github.com/repos/xperisp2020/paper-orchestra/actions/artifacts/14902098063/zip

این‌ها تست clean GitHub runner هستند، نه رایانهٔ واقعی کاربر یا Word 2010. این تفاوت را در گزارش حفظ کن.

## 3) مأموریت اول — providerهای واقعی (فقط smoke test کم‌هزینه)
کد routing موجود را بخوان؛ ابتدا بفهم SDK و نام متغیرهای محیطی دقیقاً چه هستند. سپس، اگر credentials امن از قبل در دسترس بود، برای هر provider حداکثر یک درخواست smoke کوچک انجام بده:
- Gemini: متغیر رسمی مورد استفادهٔ کد (معمولاً `GEMINI_API_KEY` یا aliasهای پشتیبانی‌شده).
- OpenAI: `OPENAI_API_KEY`.
- OpenRouter Free: `OPENROUTER_API_KEY` با شناسهٔ مدلی که واقعاً free است و در مسیر free-tier پذیرفته می‌شود.
- مدل/درخواست را ارزان و کوتاه نگه دار؛ آزمایش گسترده، batch و تکرار غیرضروری ممنوع.
- اگر provider کلید ندارد یا درخواست به هر دلیل اجرا نشد، وضعیت را `BLOCKED` یا `LIVE FAIL` با علت ثبت کن؛ آن را با mock PASS جایگزین نکن.
- هیچ‌گاه مقدار کلید یا header مجوز را چاپ نکن. حتی exception خام SDK را بدون redaction ثبت نکن.
- فقط provider، نام مدل، status code/نوع خطا، مدت زمان، زمان UTC و نتیجه را ثبت کن.
- نرخ 429 را با retry بی‌پایان دنبال نکن. تنظیم retry و pacing موجود پروژه را خراب نکن.
- اگر یک provider زنده موفق شد، فقط همان provider را `LIVE PASS` گزارش کن؛ به دیگر providerها تعمیم نده.
- بررسی کن OpenAI فقط در providerهای لازم مصرف می‌شود؛ ادعای پشتیبانی کامل pipeline را بدون اجرای واقعی مراحل نکن.
- plotting workflow مبتنی بر PaperBanana ممکن است Gemini-only باشد؛ محدودیت موجود را حذف یا دور نزن، بلکه روشن گزارش کن.

## 4) مأموریت دوم — تست روی Windows واقعی و Word 2010
اگر Windows واقعی یا VM و Word 2010 واقعی در دسترس است:
1. artifactها را از لینک‌های بالا دریافت و archive SHA-256 را دقیق مقایسه کن.
2. Portable را در مسیر دارای فاصله و حروف فارسی استخراج کن؛ اجرا، ایجاد پروژهٔ آزمایشی، ذخیره، بستن و بازگشایی را بررسی کن.
3. Installer را در مسیر سفارشی دارای فاصله/فارسی نصب کن؛ اجرا و ذخیرهٔ داده را بررسی کن؛ سپس uninstall و حفظ دادهٔ کاربر را بررسی کن.
4. از دادهٔ ساختگی استفاده کن؛ اطلاعات واقعی دانشجو/استاد وارد نکن و بخش‌های اداری را خالی نگه دار.
5. DOCX را با خود Microsoft Word 2010 باز کن. نسخهٔ کامل و bitness برنامه را ثبت کن.
6. این موارد را بررسی کن: راست‌به‌چپ و ترتیب متن فارسی، فونت و fallback، اعداد و نشانه‌گذاری، جدول‌ها، page break، سربرگ/پاورقی، حاشیه‌ها و صفحه‌بندی، باز/ذخیره مجدد، چاپ و export به PDF.
7. اسکرین‌شات یا log بدون دادهٔ حساس و مراحل بازتولید هر مشکل را ذخیره کن.
8. اگر Word 2010 نصب نیست یا مجوز/دسترسی ندارد، `BLOCKED` ثبت کن. LibreOffice یا نسخه‌های جدید Word جایگزین آزمون Word 2010 نیست.
9. اگر Windows فیزیکی کاربر در دسترس نیست، VM را «VM» بنام؛ هرگز آن را دستگاه واقعی کاربر قلمداد نکن.

## 5) مأموریت سوم — Crossref live (اختیاری و مستقل از کد مسیر A)
- helper در `utils/crossref_utils.py` وجود دارد و به Literature Agent متصل شده، اما آزمون‌های جاری mocked بوده‌اند.
- اگر شبکه به Crossref دسترسی دارد، با DOI عمومی و قابل استناد یک درخواست کم‌هزینه انجام بده؛ یک DOI شناخته‌شده و یک DOI ثبت‌نشده فقط در صورت نیاز و با رعایت rate limit.
- نتیجهٔ زنده را از mock جدا کن. خطای شبکه به معنی DOI نامعتبر نیست.
- DOI را تغییر/حذف نکن و فایل‌های مسیر A را دستکاری نکن.
- این مأموریت را در صورت عدم دسترسی شبکه BLOCKED بزن.

## 6) مرز فایل‌ها و جلوگیری از تداخل
- مسیر A (کار دستیار اصلی) در این فایل‌ها پیاده‌سازی Crossref را انجام داده است: `utils/crossref_utils.py`, `methods/agents/literature_review_agent.py`, `methods/paper_writer.py`, `methods/paper_writer_with_plotting.py`, `paper_writing_cli.py`, `app_cpp/src/main.cpp` و تست‌های متناظر.
- این فایل‌ها را تغییر نده مگر اینکه باگ مسدودکننده و قابل بازتولید پیدا کنی. ابتدا گزارش و diff ارائه کن؛ تغییر مشترک باید کوچک، هماهنگ و با بررسی HEAD/SHA جدید انجام شود.
- تغییرات قبلی خودت را حذف یا overwrite نکن. پیش از هر اقدام، مقایسه کن که آیا همان تغییر قبلاً در HEAD فعلی وجود دارد یا خیر.
- فایل `yadavarifork` را فقط append کن و رکوردهای قبلی را پاک نکن.

## 7) معیارهای نهایی تحویل
گزارش باید چهار جدول مستقل داشته باشد:
1. **موجودی تغییرات قبلی AI:** commit SHA، فایل‌ها، خلاصه diff، تست و وضعیت merge/push.
2. **Providerها:** برای Gemini/OpenAI/OpenRouter هرکدام LIVE PASS / LIVE FAIL / MOCK ONLY / BLOCKED.
3. **Windows:** Portable hash/launch/save/reopen، Installer/install/uninstall/data preservation، نوع محیط واقعی یا VM.
4. **Word 2010:** نسخه دقیق، bitness، تست‌های RTL/فونت/جدول/pagebreak/header-footer/print/PDF و نتیجه.

همچنین:
- تمام run URLها و commit SHAها را درج کن.
- log/screenshotها را بدون secrets نگه دار.
- موارد BLOCKED را با علت و پیش‌نیاز دقیق فهرست کن.
- فایل گزارش را مطابق `docs/AI_SECOND_PASS_RESULT_TEMPLATE_FA.md` کامل کن.
- فقط روی همان repo/branch مجاز write کن و دفتر `yadavarifork` را append کن.
- قبل از پایان، diff نهایی را بازبینی کن و Python Safety/CI مرتبط را در صورت تغییر کد دوباره اجرا کن.
- هیچ انتشار یا release نکن.

## ترتیب اولویت
1. افشای دقیق تغییرات قبلی و patch/commit قابل بررسی.
2. provider smoke واقعی در صورت وجود secrets.
3. Word 2010 و Windows واقعی در صورت وجود دستگاه.
4. Crossref live در صورت وجود شبکه.
5. گزارش نهایی و موارد BLOCKED.

اگر موردی از پیش قابل انجام نیست، همان لحظه BLOCKED ثبت کن و به کار بعدی برو؛ منتظر credential یا دستگاه نمان و چیزی را حدس نزن.
