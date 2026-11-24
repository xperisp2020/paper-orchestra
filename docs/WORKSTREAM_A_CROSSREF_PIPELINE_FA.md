# مسیر A — اتصال کنترل‌شدهٔ Crossref به موتور مرور ادبیات
## مسئول: دستیار اصلی

### هدف
اتصال helper موجود `utils/crossref_utils.py` به enrichment مرور ادبیات، بدون تبدیل Crossref به وابستگی اجباری، بدون حذف DOIهای ثبت‌شده نزد registrantهای دیگر و بدون توقف پژوهش هنگام اختلال شبکه.

### قرارداد رفتاری
- قابلیت به‌صورت پیش‌فرض خاموش است و فقط با گزینهٔ صریح CLI `--verify_dois_with_crossref` فعال می‌شود؛ اجرای عادی هیچ درخواست/تاخیر اضافه به Crossref ندارد.
- فقط DOI نرمال‌شده از Semantic Scholar بررسی می‌شود.
- وضعیت‌های مجاز: `verified`، `not_found`، `unavailable` و `invalid`.
- نتیجه در metadata خروجی `citation_map.json` با کلید `doi_verification_status` ثبت می‌شود.
- DOI اصلی در BibTeX و citation map حفظ می‌شود؛ 404 فقط یعنی «در Crossref یافت نشد»، نه اینکه DOI در کل نامعتبر است.
- خطا/timeout/429/پاسخ ناشناخته به `unavailable` تبدیل می‌شود و مقاله از registry حذف نمی‌شود.
- وضعیت فعال/غیرفعال در fingerprint هر دو writer قرار می‌گیرد تا resume با تنظیم متفاوت attempt قبلی را اشتباهاً بازیابی نکند.

### فایل‌ها
`methods/agents/literature_review_agent.py`, `methods/paper_writer.py`, `methods/paper_writer_with_plotting.py`, `paper_writing_cli.py`, `tests/test_literature_crossref.py`.

### آزمون و پذیرش
- تست مستقل و بدون شبکه: default-off، verified، not_found، unavailable، exception non-blocking، حفظ DOI در BibTeX/citation map و انتقال گزینه به هر دو pipeline.
- اجرای Python Safety روی Ubuntu و Windows و Research Runtime/LaTeX smoke مجدد.
- اگر endpoint زندهٔ Crossref قابل دسترسی نبود، فقط integration contract/mock PASS می‌شود؛ دسترسی زنده جداگانه BLOCKED می‌ماند.

### اجرا
```bash
python -m unittest tests.test_crossref_utils tests.test_literature_doi tests.test_literature_crossref -v
python paper_writing_cli.py --help
```

### آزمون استاندارد-library
برای اینکه Python Safety بدون نصب وابستگی‌های runtime اجرا شود، منطق non-blocking در `safe_crossref_verification_status` داخل `utils/crossref_utils.py` متمرکز است؛ تست‌های رفتاری آن در `tests/test_crossref_utils.py` هستند. فایل `tests/test_literature_crossref.py` اتصال pipeline را با AST و بررسی متن می‌سنجد و Literature Agent وابسته به Pydantic را import نمی‌کند.

- رابط C++/Qt نیز در `app_cpp/src/main.cpp` چک‌باکس اختیاری فارسی/انگلیسی دارد و در صورت انتخاب، همان گزینهٔ CLI را ارسال می‌کند؛ این گزینه به‌صورت پیش‌فرض خاموش است.
