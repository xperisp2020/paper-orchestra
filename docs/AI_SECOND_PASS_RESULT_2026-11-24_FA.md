# گزارش مرحله‌ای مسیر B — موجودی اولیه و گیت‌های باقی‌مانده
تاریخ: 2026-11-24

## 1) هویت مقصد و وضعیت شاخه
- مخزن: `xperisp2020/paper-orchestra`
- Repository ID: `1467327081`
- Owner: `xperisp2020`
- Branch مجاز: `work/yadavarifork`
- HEAD هنگام این بررسی: `de2f3f15c4eb6f7b36d06be615309ee94b0b074a`
- مخزن و شاخه با API رسمی GitHub کنترل شدند؛ شاخهٔ پیش‌فرض `main` مقصد تغییرات این مأموریت نیست.

## 2) موجودی تغییرات قبلی هوش مصنوعی دوم
**وضعیت: NO LOCAL WORKSPACE ACCESS**

- workspace/session محلی هوش مصنوعی دوم در این محیط در دسترس نیست؛ بنابراین `git status`، تغییرات staged/unstaged، فایل‌های untracked، حذف/rename و patch محلی او قابل استخراج نیست.
- هیچ patch قابل بررسی یا پیوست محلی از هوش مصنوعی دوم در این گفتگو در دسترس قرار نگرفته است.
- این نتیجه به معنی «هیچ تغییر قبلی انجام نشده» نیست؛ فقط نبود دسترسی به workspace قبلی را ثبت می‌کند.
- گزارش ممیزی موجود در مخزن نیز همین محدودیت را تصریح کرده است.
- نتیجه review تغییرات قبلی: **NOT PROVIDED**؛ تا دریافت patch یا دسترسی به همان workspace، هیچ تغییری به‌عنوان کار AI دوم تأیید یا ادغام نمی‌شود.

### بررسی تاریخچهٔ قابل مشاهده
- مقایسهٔ GitHub از checkpoint مسیر A یعنی `d68443e3d23cf9fcf435494dd9821f12e3e749d6` تا HEAD فعلی `de2f3f15c4eb6f7b36d06be615309ee94b0b074a`، دو commit و دو فایل مستنداتی را نشان داد.
- Commit `b1e4649b6f897d3641e56565922c6ab8779fdcb5`: به‌روزرسانی سند handoff V3.
- Commit `de2f3f15c4eb6f7b36d06be615309ee94b0b074a`: ثبت checkpoint نهایی مسیر A و گیت‌های باقی‌مانده در سند/دفتر.
- در این بازه هیچ source code، test یا workflow محصول تغییر نکرده است.
- author/committer برابر حساب GitHub مالک مخزن است؛ این مشخصه به‌تنهایی هویت واقعی نویسنده را (انسان یا مدل) تعیین نمی‌کند.
- شش commit مستنداتی قدیمی‌تر که در V3 فهرست شده‌اند نیز طبق diff ممیزی‌شده، فقط اسناد/دفتر را تغییر داده‌اند؛ آن‌ها مدرک پیاده‌سازی provider یا آزمون Word/Windows واقعی محسوب نمی‌شوند.

## 3) ماتریس providerهای زنده
| Provider | وضعیت فعلی | توضیح |
|---|---|---|
| Gemini | BLOCKED / در انتظار بررسی دسترسی امن | در این بررسی هیچ credential یا مقدار Secret خوانده/چاپ نشده است. |
| OpenAI | BLOCKED / در انتظار بررسی دسترسی امن | امکان مشاهدهٔ مقدار GitHub Actions secrets وجود ندارد؛ آزمون زنده بدون credential مجاز انجام نمی‌شود. |
| OpenRouter Free | BLOCKED / در انتظار بررسی دسترسی امن | نیازمند Secret ازپیش‌موجود و یک مدل رایگان معتبر؛ هیچ کلیدی در چت درخواست نمی‌شود. |

- هیچ درخواست زندهٔ Gemini/OpenAI/OpenRouter در این مرحله اجرا نشده است.
- هیچ MOCK را به‌عنوان LIVE PASS گزارش نمی‌کنیم.
- اجرای smoke زنده در ادامه فقط از مسیر Secretهای امن و بدون چاپ header، token یا خطای خام انجام خواهد شد.

## 4) Windows واقعی/VM
**BLOCKED — محیط Windows فیزیکی یا VM قابل کنترل از این نشست در دسترس نیست.**
- آخرین شواهد قابل اتکا همچنان Windows Server 2022 clean-runner در GitHub Actions با run `45367143582` است.
- این نتیجه معادل آزمون دستگاه اصلی کاربر نیست.
- artifactهای مرجع همان موارد ثبت‌شده در V3 هستند: Portable `14906139648` و Installer `14907232374`. digestهای ZIP در سند V3 درج شده‌اند.
- آزمون مستقل هش روی دستگاه فیزیکی/VM و اجرای تعاملی آن هنوز انجام نشده است.

## 5) Microsoft Word 2010
**BLOCKED — دسترسی به Microsoft Word 2010 واقعی وجود ندارد.**
- نسخه/Build و bitness واقعی Word قابل اندازه‌گیری نیست.
- CI فقط ساختار DOCX/XML، خروجی PDF، تصاویر صفحات و page-breakها را بررسی کرده است؛ این آزمون سازگاری قطعی با Word 2010 را اثبات نمی‌کند.
- LibreOffice یا Word جدیدتر جایگزین آزمون Word 2010 محسوب نمی‌شود.

## 6) Crossref live
**PASS — تکمیل‌شده در مسیر A؛ در این مأموریت نیازمند تکرار نیست مگر regression.**
- DOI عمومی: `10.1038/nature12373`
- اجراهای live: `45366306443` و `45366313174`.
- نتیجهٔ ثبت‌شده در log: `status=verified`.
- آخرین بررسی Python Safety: run `45366337200`، Ubuntu و Windows هر دو موفق با 94 تست.
- آخرین Research Runtime: run `45366337215`، Windows و Ubuntu موفق؛ 19 تست runtime در Windows و LaTeX/BibTeX واقعی در Ubuntu.
- آخرین Windows packaging/clean-runner: run `45367143582`، دو job موفق.
- لینک‌ها:
  - https://github.com/xperisp2020/paper-orchestra/actions/runs/45366306443
  - https://github.com/xperisp2020/paper-orchestra/actions/runs/45366313174
  - https://github.com/xperisp2020/paper-orchestra/actions/runs/45366337200
  - https://github.com/xperisp2020/paper-orchestra/actions/runs/45366337215
  - https://github.com/xperisp2020/paper-orchestra/actions/runs/45367143582

## 7) اقدامات این مرحله و نتیجه
- هویت repository، owner، repository ID و HEAD شاخه کنترل شد.
- سند V3، مأموریت inventory، قالب گزارش، دفتر پروژه و گزارش ممیزی خوانده شدند.
- مقایسهٔ checkpoint فعلی با HEAD نشان داد تغییرات اخیر فقط مستندات/دفتر بوده‌اند.
- فایل کد، تست یا workflow در این مرحله تغییر نکرده است.
- هیچ Secret، token یا دادهٔ شخصی به log، commit یا artifact افزوده نشده است.

## 8) گام‌های بعدی
1. فراهم‌شدن patch یا workspace هوش مصنوعی دوم و بازبینی فایل‌به‌فایل آن؛ تا آن زمان وضعیت **NO LOCAL WORKSPACE ACCESS** باقی می‌ماند.
2. بررسی وجود Secretهای لازم فقط از طریق شرط امن workflow؛ بدون خواندن یا چاپ مقدار آن‌ها؛ سپس حداکثر یک درخواست smoke کم‌هزینه برای هر provider.
3. افزودن/اجرای mockهای هدفمند timeout، 429، cancellation و redaction در مسیر مستقل، بدون ادعای live.
4. اجرای Windows واقعی/VM و Word 2010 واقعی فقط در صورت دسترسی به آن محیط‌ها؛ در غیر این صورت BLOCKED ثبت شود.
5. تکمیل همین گزارش با run URL، commit SHA و نتیجهٔ واقعی پس از هر آزمون.

**یادداشت:** این فایل گزارش مرحله‌ای است و پروژه را کامل‌شده اعلام نمی‌کند. مسیر A تأیید شده است؛ گیت‌های زندهٔ provider و آزمون‌های تعاملی Windows/Word همچنان باز هستند.


## 9) بازبینی تکمیلی تاریخچه و CI در HEAD جدید (2026-11-24)
- پیش از این ثبت، هویت مخزن با API کنترل شد: owner=`xperisp2020`, repository id=`1467327081`, نام=`paper-orchestra`؛ مقصد همچنان فقط branch `work/yadavarifork` است.
- HEAD جاری پیش از این ثبت: `6b651a052befe5bfa2a4c42fd5e4f9609b96b5a0`.
- مقایسه از checkpoint مسیر A `d68443e3d23cf9fcf435494dd9821f12e3e749d6` تا HEAD جاری: 4 commit جلوتر، 0 عقب‌تر؛ فقط این سه مسیر تغییر کرده‌اند: `docs/AI_HANDOFF_PROVIDER_WORD2010_V3_FA.md`، `docs/AI_SECOND_PASS_RESULT_2026-11-24_FA.md` و `yadavarifork`. هیچ source code، test یا workflow محصول در این بازه تغییر نکرده است.
- جدیدترین CIهای مرتبط همچنان موفق‌اند: Python Safety `45366337200`، Research Runtime `45366337215`، Crossref live `45366313174` و Windows build/clean-runner `45367143582`. این‌ها گیت‌های CI/runner هستند و جایگزین provider live با credential، Windows دستگاه/VM تعاملی یا Microsoft Word 2010 واقعی نیستند.
- وضعیت گیت‌های خارج از دسترسی تغییری نکرده است: موجودی workspace محلی AI دوم `NO LOCAL WORKSPACE ACCESS`؛ Gemini/OpenAI/OpenRouter `BLOCKED`؛ Windows واقعی/VM و Word 2010 `BLOCKED`؛ Crossref live `PASS`.
- در این بازبینی هیچ Secretی خوانده یا چاپ نشد، هیچ درخواست provider اجرا نشد، و هیچ کد محصول یا مسیر A تغییر نکرد.


## 10) پیاده‌سازی Provider Smoke و پوشش QProcess در CI (2026-11-24)

### تغییرات Commit شده
- Commit `778dbcb7370fee107edbed5b668958ab4a50eaec`:
  - `scripts/provider_live_smoke.py`: ابزار استاندارد-library برای smoke واقعی Providerها؛ حداکثر یک درخواست برای هر provider، timeout برابر 20 ثانیه، سقف پاسخ 64 KiB، بدون retry، بدون قرار دادن credential در URL/body، و بدون چاپ متن خام exception/response.
  - `tests/test_provider_live_smoke.py`: هشت تست برای نبود credential، نیاز به opt-in مدل‌های بالقوه پولی، غیرفعال‌بودن provider، رد مدل پولی OpenRouter، یک‌درخواستی بودن، 429، timeout، و عدم قرارگیری کلید در URL/body.
  - `.github/workflows/provider-live-smoke.yml`: روی push فقط مسیر OpenRouter Free را فعال می‌کند؛ Gemini و OpenAI فقط با workflow_dispatch و دو opt-in مجزا (درخواست provider و مجوز billable smoke) قابل اجرا هستند.
- Commit `276746ecf149881da41f73aac5aea2f8d759a6b1`:
  - افزودن آزمون C++ اجرایی برای cancellation و redaction در `ResearchEngineRunner` و ادغام در CMake/CTest و Windows CI.
  - اولین Build به دلیل include directory تعریف‌نشده برای target تست fail شد (run `45410733350`).
- Commit `0b06ab88ae69ed29f846756b845b94227a2a7ca6`:
  - علت Build failure اصلاح شد: include directory مسیر `app_cpp/src` به target تست افزوده شد.
  - آزمون اصلاح‌شده در run `45411318052` در حال اجراست؛ تا مشاهدهٔ نتیجه، C++ cancellation/redaction test را PASS اعلام نمی‌کنیم.

### نتایج آزمون تأییدشده
- Python Safety run `45409722980`: Ubuntu و Windows هر دو success؛ هر job تعداد 102 تست را با پیام `OK` گذراند.
  https://github.com/xperisp2020/paper-orchestra/actions/runs/45409722980
- Provider live smoke run `45409723175`: workflow/job موفق، اما هر سه provider صریحاً `BLOCKED reason=credential_missing` گزارش شدند:
  - Gemini: BLOCKED
  - OpenAI: BLOCKED
  - OpenRouter Free: BLOCKED
  هیچ درخواست provider ارسال نشد؛ نتیجهٔ workflow success به معنی Live PASS نیست.
  https://github.com/xperisp2020/paper-orchestra/actions/runs/45409723175
- Windows build + clean-runner run `45409722860`: هر دو job success؛ C++ Build، Qt app smoke، Portable/Installer assembly و اجرای clean-runner؛ خروجی PDF/DOCX در مسیر Unicode، نصب silent، اجرای اپ نصب‌شده، uninstall و حفظ sentinel داده موفق شدند.
  https://github.com/xperisp2020/paper-orchestra/actions/runs/45409722860
- artifactهای run بالا:
  - Portable id `14928290345`, size `27096591` bytes, archive digest `sha256:d31d358c367118c0cad2ababa622549b672931b657b88ecccf6bfe0ca626d870`.
  - Installer id `14926997868`, size `19933399` bytes, archive digest `sha256:374969cdaefd455635d82221a6d4d43a432d165282394f6f5e81e248ea8e0e30`.
  - Expiry ثبت‌شده: `2027-02-22T14:02:39Z`.
  - [Portable ZIP](https://api.github.com/repos/xperisp2020/paper-orchestra/actions/artifacts/14928290345/zip)
  - [Installer ZIP](https://api.github.com/repos/xperisp2020/paper-orchestra/actions/artifacts/14926997868/zip)
  - digestها برای آرشیو ZIP هستند، نه فایل اجرایی استخراج‌شده.

### وضعیت فعلی گیت‌ها
- Provider live: هر سه BLOCKED به علت credential نبودن در environment اجرای workflow؛ Gemini/OpenAI همچنین برای هر smoke نیازمند opt-in billable هستند.
- Python Safety: PASS (102 تست Ubuntu و 102 تست Windows).
- Windows GitHub clean-runner: PASS برای run `45409722860`؛ با این حال آزمون Windows دستگاه فیزیکی/VM کاربر هنوز BLOCKED است.
- C++ runner cancellation/redaction: pending CI run `45411318052`؛ اولین failure ناشی از include path بود و در commit بعدی اصلاح شده است.
- Microsoft Word 2010 واقعی: BLOCKED؛ CI ساختار Office Open XML/PDF/PNG را بررسی می‌کند ولی جایگزین بازکردن و ذخیره‌کردن در Word 2010 نیست.
- Crossref live: PASS و تکمیل‌شده در مسیر A؛ اجراهای `45366306443` و `45366313174`.
- workspace محلی AI دوم: NO LOCAL WORKSPACE ACCESS؛ patch یا فایل تغییرات محلی ارائه نشده است.
- هیچ API key/token یا مقدار Secret در این تغییرات/گزارش‌ها چاپ یا commit نشده است.
