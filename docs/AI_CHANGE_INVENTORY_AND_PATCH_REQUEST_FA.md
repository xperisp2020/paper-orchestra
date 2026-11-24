# مأموریت فوری: فهرست‌کردن و تحویل تغییرات قبلی هوش مصنوعی دوم
نسخه 1.0 — 2026-11-24

## چرا این فایل لازم است؟
در ممیزی شاخهٔ مجاز، از checkpoint قبلی تا HEAD فعلی شش commit تازه دیده شد؛ اما diff آن‌ها فقط مستندات و دفتر کار را تغییر داده است. هیچ تغییر تازه‌ای در کد محصول، تست‌های نرم‌افزاری یا workflowها در این شش commit وجود ندارد. ممکن است تغییرات ادعاشدهٔ هوش مصنوعی دوم در workspace محلی و pushنشده باقی مانده باشند. این مأموریت برای آشکارکردن همان تغییرات است؛ نه برای پاک‌کردن یا بازنویسی آن‌ها.

## محدودیت مقصد
- Repository: `xperisp2020/paper-orchestra`
- Repository id: `1467327081`
- Owner: `xperisp2020`
- تنها branch مجاز: `work/yadavarifork`
- HEAD هنگام تهیهٔ این دستورالعمل: `b08be9a11edabdc5c1e53878082de24bdd44aec9`
- هیچ repository/fork دیگری را تغییر نده.
- قبل از هر write، هویت repo، branch، HEAD و SHA فایل‌های مقصد را دوباره بررسی کن.
- از force-push، reset مخرب، حذف تاریخچه یا overwrite فایل‌های دیگران استفاده نکن.

## مرحله 1 — وضعیت واقعی workspace قبلی را آشکار کن
در همان workspace/session که تغییرات قبلی را انجام دادی، این موارد را استخراج کن:
```bash
git remote -v
git branch -vv
git status --short
git rev-parse HEAD
git log --oneline --decorate -20
git diff --stat
git diff --cached --stat
git ls-files --others --exclude-standard
```
سپس:
- تغییرات tracked را هم برای staged و هم unstaged بررسی کن.
- فایل‌های untracked را فهرست و محتوایشان را بررسی کن.
- حذف‌ها، renameها، binary assets و فایل‌های تولیدشده را جداگانه ثبت کن.
- فایل‌های secret، `.env`، credential، دادهٔ شخصی، دیتابیس شخصی، خروجی حاوی اطلاعات حساس یا log خام را وارد patch نکن.
- اگر workspace یا تاریخچهٔ قبلی دیگر در دسترس نیست، دقیقاً همین را بگو؛ ادعای «هیچ تغییری انجام نشده» نکن مگر workspace قبلی واقعاً بررسی شده باشد.

## مرحله 2 — مقایسه با پایهٔ صحیح
دو شناسه را از هم جدا کن:
- Base قدیمی که در زمان تحویل مأموریت قبلی معتبر بود: `743d19e11805e4e33f02c8133acde826187a0a31`.
- HEAD فعلی مخزن مجاز هنگام نوشتن این سند: `b08be9a11edabdc5c1e53878082de24bdd44aec9`.

اگر workspace روی Base قدیمی است:
- تفاوت‌های محلی را با هر دو base مقایسه کن.
- commitهای روی branch خودت را با current HEAD مقایسه کن.
- تغییراتی را که قبلاً در branch مجاز ادغام شده‌اند دوباره اعمال نکن.
- اگر commitهای تو در تاریخچه هستند، SHA، parent، فایل‌ها و diff را گزارش کن.

## مرحله 3 — تحویل patch بدون اعمال خودکار
اگر تغییرات commitنشده وجود دارد:
1. یک patch متنی/باینری بساز که staged و unstaged tracked changes را پوشش دهد.
2. فایل‌های untracked را جداگانه فهرست و در بستهٔ تحویل قرار بده؛ `git diff` به‌تنهایی untrackedها را شامل نمی‌شود.
3. patch را از نظر secret و دادهٔ شخصی پاک‌سازی کن.
4. هیچ patch را بی‌بررسی روی HEAD فعلی اعمال نکن.
5. SHA-256 فایل patch/بسته را در گزارش بنویس.
6. فایل patch یا بستهٔ قابل دریافت را به کاربر تحویل بده؛ اگر امکان attachment نداری، دستورهای دقیق و محتوای فایل‌های لازم را ارائه کن.

اگر تغییرات commit شده‌اند:
- URL و SHA هر commit را ارائه کن.
- به force-push یا rebase مخرب نیاز نیست؛ تغییرات را در مسیر مجاز و بدون overwrite تاریخچه تحویل بده.

## مرحله 4 — بازبینی فنی قبل از هر انتقال
برای هر فایل/تغییر:
- هدف و رفتار مورد انتظار را توضیح بده.
- وابستگی‌های جدید، API/network access، secrets، مسیرهای فایل، و platform-specific assumptions را مشخص کن.
- بررسی کن با Crossref integration مسیر A و UI Qt تداخل نداشته باشد.
- تست‌های موجود را اجرا کن؛ mock را به‌عنوان live معرفی نکن.
- اگر تغییرات کد هستند، Python Safety و workflowهای مرتبط را بعد از هماهنگی با HEAD فعلی اجرا کن.
- هیچ تغییر ناشناخته را کورکورانه merge یا commit نکن.

## خروجی اجباری
با استفاده از `docs/AI_SECOND_PASS_RESULT_TEMPLATE_FA.md` گزارش را کامل کن و این موارد را هم اضافه کن:
- Base HEAD و workspace HEAD.
- branch/remote واقعی workspace.
- تعداد فایل‌های modified/added/deleted/renamed.
- commit SHAهای قبلی.
- لیست untrackedها.
- patch یا بستهٔ قابل دانلود و SHA-256.
- آیا تغییرات قبلاً در HEAD فعلی موجودند؟
- موارد تداخل با مسیر A.
- نتیجهٔ test per change.
- وضعیت: `PATCH READY` / `ALREADY PRESENT` / `NO LOCAL WORKSPACE ACCESS` / `NO PRIOR CHANGES`.

**تا قبل از تحویل موجودی و patch، وارد تغییرات جدید محصول نشو.** پس از ارائهٔ گزارش و patch، فقط مأموریت V3 را ادامه بده.
