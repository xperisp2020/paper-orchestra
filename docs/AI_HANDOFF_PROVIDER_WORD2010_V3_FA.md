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
- **Python Safety** run `45366337200`: Ubuntu و Windows هر دو success، هرکدام 94 تست؛ شامل cache immutability و normalization تست smoke.
  https://github.com/xperisp2020/paper-orchestra/actions/runs/45366337200
- **Research Runtime** run `45366337215`: Windows Python 3.12 و Ubuntu LaTeX/BibTeX واقعی هر دو success؛ Windows runtime شامل 19 تست و تست مستقیم انتقال Crossref از هر دو Batch worker تا writer است.
  https://github.com/xperisp2020/paper-orchestra/actions/runs/45366337215
- **Crossref Live** runs `45366306443` و `45366313174`: helper واقعی با DOI عمومی `10.1038/nature12373` روی GitHub runner پاسخ `verified` دریافت کرد؛ اجرای دوم پس از اصلاح normalization ورودی smoke script بود.
  https://github.com/xperisp2020/paper-orchestra/actions/runs/45366306443
  https://github.com/xperisp2020/paper-orchestra/actions/runs/45366313174
- **Windows Build + clean-runner** run `45367143582`: Build، app smoke، Portable، Installer، استخراج و اجرای هر دو بسته در clean runner، مسیر فارسی/دارای فاصله، تولید PDF/DOCX، نصب، اجرای نسخهٔ نصب‌شده، uninstall و حفظ sentinel همگی success.
  https://github.com/xperisp2020/paper-orchestra/actions/runs/45367143582

### Artifactهای تأییدشده
- Portable artifact id `14906139648`, ZIP size `27093368` bytes, archive digest `sha256:dd4a8991fde4866163d545cf8339224c07739a920ebe6f2c8ab1b511399a45cb`.
- Installer artifact id `14907232374`, ZIP size `19931308` bytes, archive digest `sha256:57dd5238c287f47dd431676e1fa19537c80ef98c7b5851ce079860400b05b65a`.
- Expiry: `2027-02-22T09:38:40Z`.
- [Portable ZIP](https://api.github.com/repos/xperisp2020/paper-orchestra/actions/artifacts/14906139648/zip)
- [Installer ZIP](https://api.github.com/repos/xperisp2020/paper-orchestra/actions/artifacts/14907232374/zip)
- These are archive ZIP digests, not hashes of the extracted executable.

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

## 6) Crossref live — انجام شده، فقط در صورت regression باز شود
- helper واقعی پروژه با DOI عمومی `10.1038/nature12373` از داخل GitHub Actions به Crossref وصل شد و `verified` دریافت کرد.
- آزمون live پس از hardening cache در run `45366306443` و پس از اصلاح DOI URL-prefixed در smoke script در run `45366313174` نیز با success پایان یافت.
- log هر دو اجرا: `Crossref live smoke PASS: status=verified doi=10.1038/nature12373`.
- run اولیهٔ live: https://github.com/xperisp2020/paper-orchestra/actions/runs/45363848472
- run پس از cache hardening: https://github.com/xperisp2020/paper-orchestra/actions/runs/45366306443
- run پس از اصلاح normalization smoke script: https://github.com/xperisp2020/paper-orchestra/actions/runs/45366313174
- log: `Crossref live smoke PASS: status=verified doi=10.1038/nature12373`.
- این بخش در حال حاضر PASS است؛ در مأموریت فعلی دوباره‌کاری لازم نیست.
- اگر تغییر تازه‌ای در helper یا DOI normalization انجام دادی، workflow مربوطه پس از تغییر اجرا شود. خطای شبکه را invalid بودن DOI تلقی نکن و DOI را حذف/تغییر نده.

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


### یادداشت وضعیت شاخه هنگام تحویل V3
- HEAD شروع ممیزی و آماده‌سازی اسناد V3: `b08be9a11edabdc5c1e53878082de24bdd44aec9`.
- این سند و فایل موجودی در commitهای مستنداتی بعدی ثبت شده‌اند؛ برای هر عملیات جدید، HEAD واقعی را دوباره از GitHub بخوان و به SHA ثابت این سند اکتفا نکن.


### وضعیت جدید مسیر A در نسخهٔ بعدی (2026-11-24)
- Crossref live، Batch propagation و cache transient در مسیر A تکمیل شده‌اند؛ جزئیات و CIها در `docs/WORKSTREAM_A_CROSSREF_PIPELINE_FA.md` و دفتر `yadavarifork` ثبت است.
- مسیر B اکنون Crossref live را از scope عادی خود خارج می‌کند، مگر آنکه regression جدیدی رخ دهد.


### شواهد تکمیلی مسیر A (2026-11-24)
- Python Safety `45364548308`: Ubuntu/Windows success.
- Research Runtime `45364548656`: Windows runtime 18 tests `OK`; Ubuntu LaTeX/BibTeX success.
- Crossref live `45363848472`: helper واقعی پروژه status=verified برای DOI `10.1038/nature12373`.
- Windows build + clean-runner `45364368515`: success؛ artifactهای جاری Portable `14905488281` و Installer `14906087195`.


### آخرین به‌روزرسانی مسیر A و بسته‌ها (2026-11-24)
- Python Safety 94 تست Ubuntu/Windows و Research Runtime هر دو job موفق شدند؛ اجرای runtime جدید 19 تست را در Windows Python 3.12 اجرا کرد و مسیر Crossref هر دو Batch worker تا writer را PASS کرد.
- Windows run `45367143582` روی HEAD workflow `dbe12e9d32a0dc0d060a64a3f7c8a3b32c636aec` build و clean-runner هر دو را PASS کرد. این run پس از اصلاح trigger `scripts/**` اجرا شد تا هر تغییر در فایل‌های bundled scripts، Portable/Installer را rebuild کند.
- لاگ clean-runner به‌صراحت PASS بودن Portable Unicode-path smoke/PDF/DOCX و Installer install/app smoke/uninstall/user-data preservation را ثبت می‌کند.


### Checkpoint نهایی مسیر A در مخزن (2026-11-24)
- آخرین HEAD شاخه هنگام این checkpoint: `d68443e3d23cf9fcf435494dd9821f12e3e749d6`.
- مسیر A در سند `docs/WORKSTREAM_A_CROSSREF_PIPELINE_FA.md` و دفتر `yadavarifork` با تست‌ها و شواهد latest ثبت شده است.
- برای مسیر B فقط artifactهای جدید بخش 2 همین سند استفاده شوند؛ artifactهای قدیمی‌تر در تاریخچه صرفاً برای ممیزی باقی مانده‌اند.
- تغییرات بعدی AI دوم باید روی HEAD واقعی جاری rebase/patch شوند؛ force-push یا overwrite ممنوع است.


### نتیجهٔ به‌روزشده پس از اجرای چرخهٔ پذیرش UI (2026-11-24)
- CI نهایی commit `6e81d54b57bc81631767db69f50a83fc07fdf3f6` در run `45418371460` با نتیجهٔ **SUCCESS** پایان یافت؛ jobهای Build و clean-runner هر دو موفق‌اند.
- آزمون تعاملی dialog اطلاعات نهایی پروپوزال، هر دو مسیر Skip و Save، ذخیره/بازگشایی و تطبیق metadata و فیلدها را در Windows GitHub runner PASS کرد.
- Portable artifact `14933312168` با ZIP SHA-256 `a4c38def161981617c51ecfda9a701006cbb121eda68bce5a9d5e26b398a78eb`؛ Installer artifact `14933357432` با ZIP SHA-256 `b914b77dad75269388c7782707a42f1b318b9127451ec0971c080a6df0234816`؛ هر دو تا `2027-02-22T14:49:19Z` معتبرند.
- OpenRouter Free قبلاً در run `45415368110` به‌صورت live PASS تأیید شده است. Gemini/OpenAI به علت نبود credential/opt-in هزینه‌دار BLOCKED باقی می‌مانند؛ هیچ درخواست پولی ارسال نشده است.
- Windows فیزیکی/VM کاربر و Microsoft Word 2010 واقعی همچنان BLOCKED هستند؛ CI runner جایگزین آزمون روی دستگاه یا Word واقعی نیست.
- workspace محلی AI دوم همچنان در دسترس نیست؛ commitهای موجود در GitHub قابل مشاهده‌اند، اما انتساب آن‌ها به AI دوم اثبات نشده است. این محدودیت به معنی نبود تغییرات قبلی نیست.
- گزارش شواهددار و دفتر پروژه به‌روزرسانی شدند. هیچ release، انتشار عمومی، merge به main یا هزینه‌کرد انجام نشد.
