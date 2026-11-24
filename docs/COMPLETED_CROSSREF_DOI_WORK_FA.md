# مأموریت تکمیل‌شده توسط دستیار اصلی
## اعتبارسنجی مستقل DOI با Crossref

### دامنه
این بسته یک اعتبارسنجی best-effort و غیرمخرب برای DOIهای دریافتی از Semantic Scholar اضافه می‌کند. Crossref فقط DOIهای ثبت‌شده در Crossref را می‌تواند تأیید کند؛ نتیجهٔ 404 به معنی نامعتبر بودن جهانی DOI نیست.

### پیاده‌سازی
- فایل `utils/crossref_utils.py` تابع `verify_doi_with_crossref` را فراهم می‌کند.
- وضعیت‌ها: `verified`، `not_found`، `unavailable` و `invalid`.
- timeout محدود، User-Agent مشخص، امکان معرفی ایمیل اختیاری با `CROSSREF_MAILTO`، URL encoding ایمن DOI و cache حافظه‌ای با سقف 4096 مورد.
- timeout، خطای شبکه، HTTP 429/5xx، JSON خراب و DOI برگشتی نامنطبق هرگز به «DOI نامعتبر» تبدیل نمی‌شوند؛ `unavailable` برمی‌گردد.
- پاسخ 404 فقط «در Crossref یافت نشد» گزارش می‌شود و DOI حذف/تغییر نمی‌کند.
- این helper هنوز در مرحلهٔ enrichment موتور ادبیات به‌صورت خودکار فراخوانی نشده است تا هر اجرای پژوهش را به درخواست‌های شبکه‌ای اضافی و وابستگی اجباری Crossref گره نزند. برای اتصال در pipeline، ابتدا latency/rate limit و UX وضعیت‌ها را بررسی و جداگانه تست کن.

### آزمون‌ها
`tests/test_crossref_utils.py` پاسخ موفق، 404، 429، 503، timeout، DOI نامنطبق و ورودی malformed را با mock پوشش می‌دهد. تست‌ها بدون API key اجرا می‌شوند.

### فرمان آزمون
```bash
python -m unittest tests.test_crossref_utils tests.test_literature_doi -v
```

سپس کل Python Safety و Research Runtime workflow را اجرا کن. موفقیت این تست‌ها فقط رفتار helper را تأیید می‌کند؛ پاسخ زندهٔ Crossref باید در صورت نیاز با شبکهٔ واقعی و با رعایت محدودیت نرخ بررسی شود.

### محدودیت و پذیرش
- اعتبارسنجی ساختاری و Crossref از هم جدا هستند.
- Crossref پوشش‌دهندهٔ همهٔ DOIها نیست.
- در صورت outage یا عدم ثبت در Crossref، citation و DOI اصلی حفظ می‌شوند.
- هیچ DOI را صرفاً به دلیل عدم دسترسی Crossref پاک یا invalid نکن.
