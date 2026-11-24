# مأموریت مرحلهٔ دوم هوش مصنوعی — نسخه 3
تاریخ مبنا: 2026-11-24
مخزن/شاخهٔ مجاز: `xperisp2020/paper-orchestra` / `work/yadavarifork`

## 0) پیش از هر کار: موجودی تغییرات قبلی
ابتدا کل فایل `docs/AI_CHANGE_INVENTORY_AND_PATCH_REQUEST_FA.md` را اجرا کن. **تا زمانی که وضعیت workspace قبلی و patch/commitهای ادعاشده را گزارش نداده‌ای، هیچ تغییر تازه‌ای در محصول ایجاد نکن.**

نکتهٔ ممیزی جدید:
- آخرین checkpoint قبلی: `743d19e11805e4e33f02c8133acde826187a0a31`.
- شش commit تازه بعد از checkpoint در تاریخچهٔ GitHub دیده می‌شود:
  - `1ef5d536ac011dc1fad22dd3fca943ca684057ce` — گزارش audit.
  - `160a86d951901f242df64c4e9b2877acbcb1a16a` — handoff نسخهٔ 2.
  - `7c125ec77591fe4a694332ec668e77e765686808` — قالب گزارش.
  - `4e4c1c819603afad4113dbeda34393ccdaeabcf5` — علامت‌گذاری handoff قدیمی به‌عنوان superseded.
  - `157a42ee7e54e228677e7b5cc2652e1c3db54dc5` — به‌روزرسانی تقسیم کار.
  - `b08be9a11edabdc5c1e53878082de24bdd44aec9` — ثبت audit و V2.
- ممیزی diff این شش commit نشان داد فقط این مسیرها تغییر کردند: سه فایل جدید مستندات، دو فایل مستندات قبلی و دفتر `yadavarifork`. **هیچ source code، test یا workflow محصول در این شش commit تغییر نکرده است.**
- Git author/committer در commitها حساب `xperisp2020` را نشان می‌دهد؛ این مشخصه به‌تنهایی هویت نویسندهٔ واقعی (انسان یا AI) را اثبات نمی‌کند.
- بنابراین ممکن است این شش commit همان مستندات تکمیلی باشند که در ادامهٔ همکاری ساخته شده‌اند؛ اما آن‌ها شواهد پیاده‌سازی کد یا اجرای تست provider/Word نیستند.
- اگر در workspace قبلی تو تغییرات محلی/commitنشده یا branch جداگانه وجود دارد، باید آن‌ها را طبق فایل موجودی به‌صورت patch قابل بررسی تحویل بدهی. اگر workspace در دسترس نیست، صریحاً `NO LOCAL WORKSPACE ACCESS` اعلام کن.

## 1) مقصد و محدودیت غیرقابل مذاکره
- فقط repo: https://github.com/xperisp2020/paper-orchestra
- repo id: `1467327081`; owner: `xperisp2020`.
- فقط branch: `work/yadavarifork`.
- هیچ fork/repository دیگری را تغییر نده.
- پیش از هر write، owner/repo id، branch، HEAD و SHA فایل مقصد را دوباره بررسی کن.
- force-push، reset مخرب، حذف تاریخچه و overwrite تغییرات دیگران ممنوع است.
- همهٔ نتایج و تغییرات را append-only در `yadavarifork` ثبت کن.
- secrets، .env، token، دادهٔ شخصی دانشجو/استاد یا log خام حساس را در چت، commit، artifact یا patch قرار نده.
- فقط از credentials از پیش مجاز و محیط امن استفاده کن؛ کلید را در چت درخواست نکن.
- هزینه‌کرد، release، انتشار عمومی یا نصب provider با هزینه بدون تأیید صریح ممنوع است.

## 2) وضعیت پایهٔ تأییدشده — دوباره اجرا نکن مگر regression یا نیاز مشخص
گیت‌های زیر در حال حاضر PASS هستند:
- **Python Safety** run `45356267071`: Ubuntu و Windows هر دو success، هرکدام 86 تست.
  https://github.com/xperisp2020/paper-orchestra/actions/runs/45356267071
- **Research Runtime** run `45356069474`: Windows Python 3.12 bootstrap و Ubuntu TeX Live 2025/BibTeX/PDF واقعی با bibliography هر دو success.
  https://github.com/xperisp2020/paper-orchestra/actions/runs/45356069474
- **Windows Build + clean-runner** run `45356339161`: build، app smoke، Portable، Installer، اجرای مسیر فارسی/دارای فاصله، تولید PDF/DOCX، نصب، اجرای نسخه نصب‌شده، uninstall و حفظ داده sentinel همگی success.
  https://github.com/xperisp2020/paper-orchestra/actions/runs/45356339161

### Artifactهای تأییدشده
- Portable artifact id `14903375441`, ZIP size `27091376` bytes, archive digest `sha256:bd96c20e79558029fbbfe25841e6a3f54797eff50020eb5df63c8735b95d5646`.
- Installer artifact id `14902098063`, ZIP size `19929671` bytes, archive digest `sha256:f86742f3136f3344422fcda05a5798789bec3209fa94f4b532c6a41ada36bca5`.
- Expiry: `2027-02-22T08:27:57Z`.
- [Portable ZIP](https://api.github.com/repos/xperisp2020/paper-orchestra/actions/artifacts/14903375441/zip)
- [Installer ZIP](https://api.github.com/repos/xperisp2020/paper-orchestra/actions/artifacts/14902098063/zip)

این نتایج روی Windows Server 2022 clean runner است؛ نه دستگاه اصلی کاربر و نه Word 2010 واقعی. hashهای بالا digest آرشیو ZIP هستند.

## 3) Providerهای زنده — مأموریت اصلی
کد routing را بخوان و فقط اگر credential امن و مجاز از قبل موجود است، برای هر provider حداکثر یک درخواست smoke کوتاه و کم‌هزینه بفرست:
- Gemini: متغیرهای محیطی رسمی مورد استفادهٔ کد.
- OpenAI: `OPENAI_API_KEY`.
- OpenRouter Free: `OPENROUTER_API_KEY` و مدل رایگان معتبر که کد آن را بپذیرد.
قواعد:
- هیچ کلید یا Authorization header چاپ نشود؛ exception خام بدون redaction ممنوع.
- وضعیت هر provider مستقل: `LIVE PASS`, `LIVE FAIL`, `BLOCKED`.
- mock tests جداگانه گزارش شوند؛ mock هرگز LIVE PASS نیست.
- برای 429 retry نامحدود نداشته باش؛ pacing و retry موجود را حفظ کن.
- پشتیبانی کامل OpenAI را بدون اجرای واقعی مراحل pipeline ادعا نکن.
- PaperBanana plotting مسیر Gemini-only دارد؛ این محدودیت را حذف یا دور نزن.

## 4) Windows واقعی یا VM
اگر Windows واقعی یا VM در دسترس است:
1. artifactهای بالا را بگیر و archive SHA-256 را مقایسه کن.
2. Portable را در مسیر دارای فاصله و حروف فارسی اجرا کن؛ ایجاد پروژه، ذخیره، بستن و بازکردن مجدد.
3. Installer را در مسیر سفارشی فارسی/دارای فاصله نصب و اجرا کن؛ uninstall و حفاظت داده را بررسی کن.
4. فقط دادهٔ ساختگی استفاده کن.
5. نوع محیط را دقیق بنویس: دستگاه واقعی یا VM؛ یکی را به‌جای دیگری گزارش نکن.
6. برای خطاها Windows build، architecture، steps، screenshot و logs پاک‌سازی‌شده ثبت کن.
این گیت‌های clean-runner در CI قبلاً PASS شده‌اند؛ این مأموریت برای محیط واقعی/دستگاه کاربر است، نه اجرای تکراری همان workflow بدون دلیل.

## 5) Microsoft Word 2010 — گیت مستقل
DOCX تولیدشده را فقط در Microsoft Word 2010 واقعی باز کن و نسخه/build و 32/64-bit را ثبت کن. RTL فارسی، ترتیب، فونت/fallback، اعداد، جدول، page break، header/footer، margin، page layout، باز/ذخیره مجدد، print و PDF را بررسی کن.
- Word 2010 نبود: `BLOCKED`.
- LibreOffice یا Word جدیدتر معادل Word 2010 نیست.
- اگر Word روی VM نصب است، نتیجه را `Word 2010 on VM` بنویس؛ نه دستگاه اصلی کاربر.

## 6) Crossref live — گیت جدا
helper موجود است و Crossref در Literature Agent به‌صورت opt-in وصل شده. Mock tests وجود دارند؛ اما درخواست زندهٔ Crossref هنوز تأیید نشده.
- اگر شبکه در دسترس است، یک DOI عمومی که از Semantic Scholar دریافت شده را از طریق helper بررسی کن.
- فقط status، HTTP، زمان و خروجی غیرحساس ثبت شود.
- `not_found` یعنی در Crossref یافت نشد، نه invalid globally؛ DOI را حذف/تغییر نده.
- اگر شبکه در دسترس نیست، `BLOCKED` ثبت کن.

## 7) جلوگیری از تداخل با مسیر A
فایل‌های مسیر A که تا پیش از این مأموریت تغییر کرده‌اند:
- `utils/crossref_utils.py`
- `methods/agents/literature_review_agent.py`
- `methods/paper_writer.py`
- `methods/paper_writer_with_plotting.py`
- `paper_writing_cli.py`
- `app_cpp/src/main.cpp`
- تست‌های Crossref مربوطه.
این فایل‌ها را تغییر نده مگر مشکل مسدودکنندهٔ قابل بازتولید پیدا کنی. قبل از اصلاح، diff و شواهد را گزارش کن و از کاربر/مسیر A تأیید هماهنگی بگیر. کد مسیر A را بدون هماهنگی overwrite نکن.

## 8) گزارش اجباری
گزارش را در `docs/AI_SECOND_PASS_RESULT_TEMPLATE_FA.md` تکمیل کن:
- موجودی تغییرات قبلی و patch/commit قابل بررسی.
- وضعیت live هر provider.
- Windows واقعی یا VM و نتایج هر گام.
- Word 2010 version/build/bitness و تست‌های جزئی.
- Crossref live.
- خطاها، علت ریشه‌ای، اصلاح و commit SHA.
- موارد `BLOCKED` با علت و پیش‌نیاز دقیق.
- run URLs، artifact IDs، hashes و آخرین HEAD.
- اطمینان از عدم افشای secrets.

پس از تکمیل، دفتر `yadavarifork` را append کن؛ سابقه را پاک نکن. پیش از پایان، diff نهایی را بازبینی کن. هیچ release یا انتشار عمومی انجام نده.

## ترتیب اجباری
1. ابتدا فهرست و patch تغییرات قبلی (فایل `AI_CHANGE_INVENTORY_AND_PATCH_REQUEST_FA.md`).
2. سپس provider smokeهای live در صورت دسترسی.
3. سپس Word 2010 و Windows واقعی/VM در صورت دسترسی.
4. سپس Crossref live در صورت دسترسی.
5. در پایان گزارش شواهددار و BLOCKEDها.
