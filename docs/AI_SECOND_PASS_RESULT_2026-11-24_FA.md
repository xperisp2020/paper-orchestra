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


## 11) نتیجهٔ نهایی اجرای C++ cancellation/redaction و بسته‌بندی (2026-11-24)

### اصلاح و آزمون نهایی
- علت failure اولیهٔ C++ run `45410733350` در log مشخص بود: compiler نمی‌توانست `research_engine_runner.h` را از target تست پیدا کند (include directory تعریف نشده بود).
- در commit `0b06ab88ae69ed29f846756b845b94227a2a7ca6` مسیر `app_cpp/src` به include directories target افزوده شد؛ تاریخچه حفظ شد و هیچ force-push انجام نشد.
- Windows CI run `45411318052` روی همین commit با هر دو job success پایان یافت:
  - Build/CTest/Portable/Installer: PASS.
  - Clean-runner artifact verification: PASS.
- لاگ CTest:
  - `app_smoke`: Passed (8.58s).
  - `research_engine_runner_contract`: Passed (5.15s).
  - `100% tests passed, 0 tests failed out of 2`.
- آزمون C++ runner به‌صورت runtime فرایند فرزند می‌سازد؛ خروجی دارای متغیر محیطی، Bearer header و x-api-key را از signal خروجی بررسی می‌کند؛ start دوم هنگام running رد می‌شود و cancellation واقعی از terminate/kill lifecycle عبور می‌کند.
- لاگ clean runner:
  - `Clean-runner portable smoke, proposal save/reopen, and Unicode-path exports passed.`
  - `Installer install, app smoke, uninstall, and user-data preservation passed.`
- لینک اجرا: https://github.com/xperisp2020/paper-orchestra/actions/runs/45411318052

### Artifactهای جدید و مرجع
- Portable artifact id `14930080169`, size `27096591` bytes, archive digest `sha256:5c8677a86d1cc4dace61e4d523aeb406124ce5b9a424f0372ccbbace8bc59719`.
- Installer artifact id `14930015297`, size `19933400` bytes, archive digest `sha256:1ce08a97fcfbc7b3a595a6765549da7b01e59b5d46cd09d131c02999dea820d0`.
- Expiry: `2027-02-22T14:11:25Z`.
- [Portable ZIP](https://api.github.com/repos/xperisp2020/paper-orchestra/actions/artifacts/14930080169/zip)
- [Installer ZIP](https://api.github.com/repos/xperisp2020/paper-orchestra/actions/artifacts/14930015297/zip)
- digestها مختص آرشیو ZIP هستند و hash فایل اجرایی داخل archive نیستند.

### ماتریس گیت‌ها پس از این دور
| Gate | Result | Evidence |
|---|---|---|
| Python Safety (provider-smoke suite) | PASS — 102 tests on Ubuntu and Windows | `45409722980` |
| Provider live harness | Harness PASS; Gemini/OpenAI/OpenRouter each BLOCKED: credential_missing | `45409723175` |
| C++ ResearchEngineRunner cancellation/redaction | PASS | `45411318052`, CTest 2/2 |
| Windows GitHub clean-runner Portable/Installer | PASS | `45411318052` |
| Crossref live | PASS — completed in Workstream A | `45366306443`, `45366313174` |
| Prior AI2 local workspace/patch | NO LOCAL WORKSPACE ACCESS | No patch supplied |
| User's physical Windows device / VM | BLOCKED | No interactive device/VM available |
| Microsoft Word 2010 real application | BLOCKED | No Word 2010 environment available |
| Gemini/OpenAI paid live | BLOCKED | Missing workflow credential; additionally requires explicit billable opt-in |

### نتیجهٔ صادقانه
- بخش قابل اجرا از سمت دستیار تکمیل و CI شده است: provider smoke harness امن، mock coverage افزوده‌شده، QProcess cancellation/redaction runtime test، و بسته‌بندی جدید Windows.
- سه provider در این نوبت live-tested نشده‌اند؛ هیچ credential در job environment حاضر نبود و به همین علت هیچ درخواست واقعی ارسال نشد. workflow به‌صورت دستی و با Secretهای امن قابل اجرای مجدد است؛ Gemini/OpenAI به opt-in پرداختی صریح نیاز دارند.
- PASS بودن clean runner معادل اجرای روی دستگاه اصلی کاربر نیست. Word 2010 نیز با CI XML/PDF validation اثبات نمی‌شود.
- نتیجهٔ کامل پروژه هنوز **BLOCKED** است تا گیت‌های محیط واقعی و provider زنده (در صورت فراهم بودن Secret مجاز) تعیین تکلیف شوند.
- Secret/API key/token خوانده یا در log/commit/artifact چاپ نشده است.


## 12) اجرای کامل موجودی V3 و ممیزی checkpoint تا HEAD فعلی (2026-11-24)

### وضعیت workspace قبلی
- فایل مأموریت `docs/AI_CHANGE_INVENTORY_AND_PATCH_REQUEST_FA.md` به‌طور کامل بررسی شد و همهٔ مراحل آن اعمال شد.
- workspace/session محلی AI دوم در این نشست mount یا متصل نیست؛ بنابراین `git remote -v`، `git status --short`، staged/unstaged diff و untrackedهای آن workspace قابل اجرا/مشاهده نیستند.
- نتیجهٔ دقیق: **NO LOCAL WORKSPACE ACCESS**؛ این نتیجه به معنی `NO PRIOR CHANGES` نیست.
- هیچ patch محلی یا بستهٔ قابل ممیزی از workspace قبلی در دسترس نیست؛ بنابراین patch، تعداد فایل‌های محلی، SHA-256 patch و ادعای تغییرات commitنشده قابل ارائه نیست.
- تغییرات commit‌شدهٔ قابل مشاهده در branch مجاز جداگانه با GitHub compare بررسی شدند و نباید با patch محلی/commitنشدهٔ workspace قبلی اشتباه گرفته شوند.

### مقایسهٔ checkpoint و تاریخچه
- Base/checkpoint مسیر A: `d68443e3d23cf9fcf435494dd9821f12e3e749d6`.
- HEAD پیش از ثبت این inventory: `fcc508910de9c5e4e2c655d12cea642b40a06ce7`.
- GitHub compare: **14 commits ahead, 0 behind**.
- 14 commit قابل مشاهده:
  - `b1e4649b6f897d3641e56565922c6ab8779fdcb5` — Add final Workstream A checkpoint to V3 handoff
  - `de2f3f15c4eb6f7b36d06be615309ee94b0b074a` — Record final Workstream A checkpoint and remaining project gates
  - `bb2b6520285684413f950a40911aa5fb96dc2b25` — Add route B inventory and initial verification report
  - `6b651a052befe5bfa2a4c42fd5e4f9609b96b5a0` — Record initial route B inventory and blockers
  - `f6489c24a51acf5af151005b0d1d137a9bb5df2e` — docs: append V3 route B verification addendum
  - `77037c00247f6b06e4188777987321e3788cac36` — docs: append V3 route B audit checkpoint
  - `778dbcb7370fee107edbed5b668958ab4a50eaec` — Add guarded provider live smoke workflow and tests
  - `276746ecf149881da41f73aac5aea2f8d759a6b1` — Add QProcess cancellation and redaction regression tests
  - `0b06ab88ae69ed29f846756b845b94227a2a7ca6` — Fix runner test target include directory
  - `f483aacb4c224a7e7e2e13177a22b128c4a23a32` — Record provider smoke and QProcess test progress
  - `691725d7a540a361b8de412240809051180a4f49` — Append provider smoke and QProcess test checkpoint
  - `4534182b62ea6a83f468053c38a190554dfb47aa` — Record passing QProcess cancellation and Windows clean-runner CI
  - `8faef4192db1a3edacc76ab66242d228df59e44c` — Append passing QProcess and Windows CI checkpoint
  - `fcc508910de9c5e4e2c655d12cea642b40a06ce7` — ci: document branch-scoped provider smoke workflow
- Diff نهایی این بازه 9 مسیر را نشان داد: 1 workflow موجود اصلاح‌شده، 1 workflow جدید، 1 فایل CMake اصلاح‌شده، 1 تست C++ جدید، 2 سند handoff/report، 1 اسکریپت provider smoke جدید، 1 فایل تست Python جدید و دفتر پروژه.
- بنابراین این بازه صرفاً مستنداتی نیست: provider harness و تست‌های QProcess/C++ نیز در commitهای قابل مشاهده وجود دارند. این موارد در بخش‌های 10 و 11 گزارش با commit و CI ثبت شده‌اند.
- هیچ تغییر حذف/rename در compare دیده نشد. فایل‌های تغییرکرده:
  - `.github/workflows/cpp-portable-ci.yml`
  - `.github/workflows/provider-live-smoke.yml`
  - `app_cpp/CMakeLists.txt`
  - `app_cpp/tests/research_engine_runner_test.cpp`
  - `docs/AI_HANDOFF_PROVIDER_WORD2010_V3_FA.md`
  - `docs/AI_SECOND_PASS_RESULT_2026-11-24_FA.md`
  - `scripts/provider_live_smoke.py`
  - `tests/test_provider_live_smoke.py`
  - `yadavarifork`

### نتیجهٔ inventory
- وضعیت workspace قبلی: **NO LOCAL WORKSPACE ACCESS**.
- وضعیت commitهای موجود در branch مجاز: **ALREADY PRESENT**؛ commitهای بالا در HEAD قابل مشاهده و به‌صورت تفکیک‌شده audit شدند.
- patch محلی قابل انتقال: **NOT AVAILABLE**؛ هیچ patch ناشناخته‌ای اعمال یا overwrite نشد.
- تغییرات مسیر A دست‌نخورده باقی ماندند.
- این inventory پیش از شروع اصلاح تازهٔ Provider ثبت شد؛ secrets/credential در آن درج نشده است.


## 13) عیب‌یابی Provider، اصلاح OpenRouter و چرخهٔ آزمون مجدد (2026-11-24)

### علت‌یابی failure اولیه
- Provider run `45413396317` روی commit `fcc508910de9c5e4e2c655d12cea642b40a06ce7` برای OpenRouter پاسخ HTTP 200 گرفت، اما harness متن مورد انتظار را تشخیص نداد و `LIVE FAIL reason=empty_or_unexpected_response` ثبت کرد.
- بررسی request نشان داد smoke برای همهٔ Chat Completions فقط `max_tokens=8` می‌فرستاد؛ در حالی که مدل پیش‌فرض `nvidia/nemotron-3.5-super-vl:free` reasoning را به‌طور پیش‌فرض فعال می‌کند. مستند رسمی OpenRouter می‌گوید reasoning را می‌توان با پارامتر `reasoning` کنترل کرد؛ مستند مدل نیز همین مدل را رایگان و OpenRouter-compatible معرفی می‌کند.
- اصلاح کم‌هزینه: برای OpenRouter مقدار `reasoning.effort=none` اضافه شد و سقف پاسخ کوتاه از 8 به 16 token افزایش یافت. هیچ retry، درخواست دوم در همان probe، یا provider پولی اضافه نشد.

### اصلاح و آزمون‌ها
- Commit اصلاحی: `9412064616b8e5bc7f51be02fc3674dc62b077b3`؛ اسکریپت `scripts/provider_live_smoke.py` و تست `tests/test_provider_live_smoke.py` تغییر کردند. تست request اکنون وجود `reasoning.effort=none` و `max_tokens=16` را بررسی می‌کند.
- یادداشت شفافیت تاریخچه: هنگام ساخت Git tree اصلاحی، base tree به‌درستی به ابزار داده نشده بود و commit `9412064...` موقتاً tree ناقصی ساخت. بلافاصله commit بازیابی `175d9075bff98edaf889ab83e818b90ada4171cd` با parent tree کامل ساخته شد؛ compare از parent قبلی `676919a9c6fe5aa1d25fcbc595a5cc6b4b184c03` تا commit بازیابی فقط دو فایل اسکریپت و تست Provider را modified نشان می‌دهد. tree بازگشتی 137 entry داشت و README، CMake، workflowها، اسناد و فایل‌های اصلی دوباره از GitHub خوانده و موجود بودنشان تأیید شد. هیچ force-push یا reset مخرب انجام نشد. هر دو commit در تاریخچه باقی مانده‌اند.
- Commit توضیحی برای trigger کردن workflow: `5f8fc56578a9297dba57f32ec88f83eab4069365`.
- Provider run `45415368110`: **PASS در سطح job**؛ OpenRouter Free نتیجهٔ `LIVE PASS`، مدل `nvidia/nemotron-3.5-super-vl:free`، HTTP 200، زمان 478ms. Gemini و OpenAI به‌صورت عمدی `BLOCKED reason=not_enabled` باقی ماندند؛ هیچ آزمون پولی انجام نشد.
- Python Safety run `45415328764`: Ubuntu و Windows هر دو success؛ هر کدام **102 تست** اجرا کردند.
- Research Runtime run `45415328732`: job ویندوز bootstrap و job Ubuntu LaTeX/BibTeX هر دو success.
- Portable Windows CI run `45415328651`: success.
- Windows MiKTeX smoke run `45415328550`: success؛ PDF مبتنی بر bibliography واقعی در runner ویندوز ساخته شد.
- C++ Windows portable CI run `45415328570`: هر دو job Build و clean-runner success؛ CTest شامل `app_smoke` و `research_engine_runner_contract`، نتیجه `100% tests passed, 0 tests failed out of 2`. clean runner موفقیت Portable save/reopen و export در مسیر Unicode، و Installer install/app smoke/uninstall/user-data preservation را ثبت کرد.
- Artifactهای همین run:
  - Portable id `14932002540`, size 27096756 bytes, ZIP SHA-256 `c1b24adea5081f8000441b8188e94ce8e65667647f9d0939053d55e0ec2ed473`.
  - Installer id `14932192574`, size 19933340 bytes, ZIP SHA-256 `ed0d829e98e6e2a1c0cd614bee899cb31f48bcdcb38c4f2db6404d727bf9cd61`.
  - هر دو artifact تا `2027-02-22T14:33:01Z` معتبرند؛ digestها مربوط به ZIP هستند.
- Crossref live run `45415328728`: success؛ مسیر A بدون تغییر باقی ماند.

### تغییرات هم‌زمان بعدی در شاخه
- دو commit موازی روی همان شاخه و توسط جریان کاری دیگری ثبت شد:
  - `2398f960066a9746d97eb55f0b1eb2a2cf5e1562` — افزودن پوشش محتوای فارسی و RTL در DOCX.
  - `0e9cc4ae54860c479eb8fa0fb69ad5592310df3f` — اعتبارسنجی XML مربوط به RTL و خالی‌ماندن فیلدهای اداری.
- این تغییرات فقط بازبینی و حفظ شدند؛ هیچ‌کدام overwrite نشدند. C++ Windows CIهای مرتبط `45415689848` و `45415730486` هنگام آخرین بررسی در حال اجرا بودند؛ تا نتیجهٔ نهایی آن‌ها PASS جدید برای این تغییرات ادعا نمی‌شود.

### وضعیت گیت‌ها پس از چرخهٔ اصلاح
| Gate | Result |
|---|---|
| OpenRouter Free live smoke | **LIVE PASS** — run `45415368110` |
| Gemini live smoke | **BLOCKED** — عمداً not enabled؛ opt-in هزینه‌دار داده نشده |
| OpenAI live smoke | **BLOCKED** — عمداً not enabled؛ opt-in هزینه‌دار داده نشده |
| Python Safety | **PASS** — 102 tests در Ubuntu و Windows |
| Research Runtime / LaTeX-BibTeX | **PASS** |
| Windows GitHub build + clean-runner | **PASS** — run `45415328570` |
| Windows MiKTeX real bibliography PDF smoke | **PASS** — run `45415328550` |
| Crossref live | **PASS** — run `45415328728` |
| Windows دستگاه فیزیکی یا VM کاربر | **BLOCKED** — محیط تعاملی در دسترس نیست |
| Microsoft Word 2010 واقعی | **BLOCKED** — نسخه/build/bitness و آزمون باز/ذخیره/چاپ قابل دسترسی نیست |
| workspace محلی AI دوم | **NO LOCAL WORKSPACE ACCESS** |

- هیچ Secret یا Authorization header در log/commit/artifact چاپ نشد. پاسخ provider و متن exception خام ذخیره یا نمایش داده نشد.
- هیچ Gemini/OpenAI پولی، Release، انتشار عمومی، merge به `main` یا تغییر مسیر A انجام نشد.
- وضعیت پروژه هنوز **کامل نیست**؛ گیت‌های Word 2010 و Windows فیزیکی/VM و opt-in هزینه‌دار Gemini/OpenAI باز هستند.


## 14) تکمیل آزمون‌های تغییرات RTL و artifactهای تازه (2026-11-24)

- دو commit موازی RTL که در بخش 13 ثبت شدند، اکنون CI نهایی خود را نیز گذرانده‌اند:
  - `45415689848` برای commit `2398f960066a9746d97eb55f0b1eb2a2cf5e1562`: Build و clean-runner هر دو success.
  - `45415730486` برای commit `0e9cc4ae54860c479eb8fa0fb69ad5592310df3f`: Build و clean-runner هر دو success.
- در commit دوم، smoke خروجی DOCX وجود متن فارسی/کاراکترهای escapeشده، ویژگی‌های RTL یعنی `bidi` و `rtl`، و خالی‌بودن فیلدهای اداری تعریف‌شده را assert کرد. CTest نیز `app_smoke` و `research_engine_runner_contract` را PASS کرد؛ نتیجه `100% tests passed, 0 tests failed out of 2`.
- clean-runner run `45415730486` موفق شد Portable را در مسیر Unicode باز کند، proposal را ذخیره/بازگشایی کند و export بگیرد؛ Installer را نصب و اجرا و uninstall کرد و حفظ دادهٔ کاربر را تأیید کرد.
- Artifactهای نهایی commit RTL:
  - Portable id `14932431892`, size 27096778 bytes, ZIP SHA-256 `ed2f760dedd4aab2ff6c87663b2ee25219942f1853ac18dd85e9603e679d5807`.
  - Installer id `14931477161`, size 19933308 bytes, ZIP SHA-256 `c7fcd0ec5d821fb6385ee89052080adfbd911f3d7b3f5599b70ce5cea6cc0819`.
  - Expiry ثبت‌شده: `2027-02-22T14:35:08Z`; digestها متعلق به ZIP هستند.
- بنابراین C++ Windows build و clean-runner در آخرین commit RTL نیز **PASS** است. این محیط Windows GitHub runner است و به معنی آزمون Windows فیزیکی/VM کاربر یا Word 2010 واقعی نیست.
- آخرین HEAD شاخه هنگام تکمیل این گزارش: `4f2cceac4128cb6e2653d0f4185b4d1575c4aaf9` (ثبت افزایشی دفتر پروژه).


## 15) تقویت آزمون تعاملی فرم اطلاعات نهایی پروپوزال (2026-11-24)

- در commit `db5496d53afd31ebbd2562a965863dbcd08809a2` آزمون smoke از دستکاری مستقیم لایهٔ ذخیره‌سازی به تعامل با dialog واقعی تغییر کرد.
- آزمون اکنون دکمهٔ واقعی «فعلاً رد شو؛ بعداً تکمیل می‌کنم» را کلیک می‌کند، وضعیت `skipped` را از metadata بازمی‌خواند و خالی‌ماندن بخش‌های اطلاعات دانشجو/استاد را کنترل می‌کند.
- مسیر ذخیره نیز dialog واقعی را باز می‌کند، مقادیر چندخطی و فارسی/RTL را در کنترل‌های UI وارد می‌کند، دکمهٔ Save را می‌فشارد، پروژه را مجدداً باز می‌کند و ماندگاری اطلاعات را بررسی می‌کند.
- برای کنترل‌های فرم objectNameهای پایدار اضافه شد تا آزمون UI بتواند فیلدها را بدون وابستگی به ترتیب فرزندان پیدا کند.
- تغییر فقط در `app_cpp/src/main.cpp` انجام شد؛ branch head پس از commit برابر `db5496d53afd31ebbd2562a965863dbcd08809a2` تأیید شد.
- در زمان ثبت این یادداشت، ابزار وضعیت/اجرای Actions برای این commit نتیجهٔ قابل مشاهده‌ای برنگرداند؛ بنابراین این تغییر **منتظر تأیید CI** است و PASS ادعا نمی‌شود. آزمون Word 2010 و دستگاه فیزیکی/VM همچنان خارج از این دامنه و مسدود هستند.


### تکمیل assertionهای پذیرش ذخیرهٔ مشخصات پروپوزال
- commit کد تکمیلی: `6e81d54b57bc81631767db69f50a83fc07fdf3f6`؛ فقط `app_cpp/src/main.cpp` تغییر کرد.
- پس از ذخیره از طریق dialog واقعی و بازگشایی پروژه، smoke اکنون علاوه بر تطبیق تمام فیلدهای نهایی، خالی‌بودن خطای ذخیره‌سازی و metadata برابر `final_details_status=completed` را نیز assert می‌کند.
- درخواست فهرست workflowهای Actions با `head_sha` این commit در زمان بررسی `total_count=0` برگرداند؛ بنابراین trigger/اجرای CI هنوز مشاهده نشده و نتیجه همچنان **PENDING** است. هیچ PASS جدیدی ادعا نمی‌شود.


### تأیید نهایی CI برای آزمون تعاملی فرم اطلاعات پروپوزال (2026-11-24)
- CI برای commit کد `6e81d54b57bc81631767db69f50a83fc07fdf3f6` در run `45418371460` اکنون **COMPLETED / SUCCESS** است.
- هر دو job موفق شدند: `build-windows` و `verify-portable-artifact`.
- تمام مراحل build job موفق بودند: Configure، Build، Smoke test، ساخت Portable، بارگذاری Portable، نصب compiler نسخه‌ثابت Inno Setup و بارگذاری Installer.
- clean-runner هر دو artifact را دریافت، استخراج و بررسی کرد و موفق شد.
- این اجرا assertionهای جدید مسیر Save را نیز روی Windows runner اجرا کرد: ذخیره از dialog واقعی، بازگشایی پروژه، تطبیق فیلدها، نبود خطای ذخیره‌سازی و metadata برابر `final_details_status=completed`. مسیر Skip و خالی‌ماندن فیلدهای اداری نیز در smoke تعاملی پوشش داده شده است.
- لینک اجرا: https://github.com/xperisp2020/paper-orchestra/actions/runs/45418371460
- Artifactهای دقیق همین commit:
  - Portable id `14933312168`, حجم 27,098,091 bytes، ZIP SHA-256 `a4c38def161981617c51ecfda9a701006cbb121eda68bce5a9d5e26b398a78eb`.
  - Installer id `14933357432`, حجم 19,934,286 bytes، ZIP SHA-256 `b914b77dad75269388c7782707a42f1b318b9127451ec0971c080a6df0234816`.
  - انقضای هر دو: `2027-02-22T14:49:19Z`. SHA-256ها مربوط به فایل ZIP هستند.
- لینک‌ها:
  - [Portable ZIP](https://api.github.com/repos/xperisp2020/paper-orchestra/actions/artifacts/14933312168/zip)
  - [Installer ZIP](https://api.github.com/repos/xperisp2020/paper-orchestra/actions/artifacts/14933357432/zip)
- این CI تأیید اجرای خودکار روی Windows clean runner است؛ آزمون روی دستگاه فیزیکی/VM کاربر و باز/ذخیره/چاپ در Microsoft Word 2010 واقعی را اثبات نمی‌کند.
- نتیجه: گیت CI برای آزمون تعاملی فرم و بسته‌بندی Windows **PASS**؛ گیت‌های محیط واقعی Windows/VM و Word 2010 همچنان **BLOCKED / نیازمند محیط واقعی** هستند. Gemini/OpenAI پولی نیز بدون مجوز مالی صریح فعال نشده‌اند.
