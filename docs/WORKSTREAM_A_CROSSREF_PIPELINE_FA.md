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

### اعتبارسنجی UI و بسته‌بندی بعد از اتصال
- رابط Qt یک چک‌باکس اختیاری فارسی/انگلیسی برای Crossref دارد و فقط در صورت انتخاب، گزینهٔ CLI را ارسال می‌کند.
- Python Safety run 45356267071 روی Ubuntu و Windows موفق بود (86 تست). C++/Windows run 45356339161 نیز ساخت، smoke test، Portable/Installer و clean-runner verification را با موفقیت گذراند.
- run 45356257643 ابتدا در نصب Installer شکست خورد؛ علت، quoting مسیر فارسی/دارای فاصله در آرگومان /DIR بود و در commit f1d5cb39d90afb645d3820ab83c887eb5cbbcba2 اصلاح شد. اجرای مجدد موفق run 45356339161 بود.
- دستگاه واقعی کاربر، Microsoft Word 2010 واقعی، provider live و Crossref live هنوز تأیید نشده‌اند.

### تکمیل مسیر Batch و cache خطاهای موقت
- گزینهٔ Crossref اکنون از `run_batch_writeup` در هر دو writer به workerهای `ProcessPoolExecutor` منتقل می‌شود؛ پیش‌فرض همچنان خاموش است.
- پاسخ‌های قطعی `verified` و `not_found` در LRU process-local cache می‌شوند؛ خطاهای موقت `unavailable` (timeout/429/5xx/پاسخ نامعتبر) از طریق exception داخلی از cache عبور نمی‌کنند و در فراخوانی بعدی دوباره قابل تلاش‌اند.
- timeout نامعتبر/غیرمثبت به `unavailable` تبدیل می‌شود و درخواست شبکه نمی‌فرستد.
- README مثال و محدودیت Crossref را مستند می‌کند؛ status در citation_map، DOI اصلی را تغییر نمی‌دهد.
- تست‌های جدید: بازیابی پس از HTTP 503 و propagation گزینه به هر دو batch worker.

### آزمون زندهٔ مستقل Crossref
- workflow جدید `.github/workflows/crossref-live-smoke.yml` فقط هنگام تغییر helper/normalizer/script یا اجرای دستی فعال می‌شود.
- `scripts/crossref_live_smoke.py` یک درخواست عمومی با DOI شناخته‌شدهٔ `10.1038/nature12373` می‌فرستد؛ هیچ credential لازم نیست و فقط status و DOI عمومی در log چاپ می‌شود.
- این workflow از تست‌های mock و Python Safety جداست؛ نتیجهٔ PASS آن به معنی دسترسی endpoint از GitHub runner است، نه تضمین دسترسی از تمام شبکه‌های کاربر.

### نتایج تأییدشدهٔ مسیر A پس از اصلاح Batch و cache
- Python Safety run `45363478938`: Ubuntu و Windows هر دو success؛ هر کدام 89 تست. شامل آزمون انتقال Crossref به دو batch worker و retry پس از HTTP 503.
  https://github.com/xperisp2020/paper-orchestra/actions/runs/45363478938
- Research Runtime run `45363463950`: bootstrap Windows و LaTeX/BibTeX واقعی روی Ubuntu هر دو success.
  https://github.com/xperisp2020/paper-orchestra/actions/runs/45363463950
- Live Crossref run `45363848472`: success؛ log صریحاً `Crossref live smoke PASS: status=verified doi=10.1038/nature12373` را ثبت کرد.
  https://github.com/xperisp2020/paper-orchestra/actions/runs/45363848472
- live smoke یک درخواست عمومی و کم‌هزینه با DOI شناخته‌شده انجام می‌دهد؛ این فقط دسترسی helper از GitHub runner را ثابت می‌کند و دسترسی تمام دستگاه‌ها/شبکه‌ها را تضمین نمی‌کند.
- C++/Windows بسته‌بندی با کد منبع Batch و Crossref در workflow جاری `45363487717` هنوز باید به conclusion نهایی برسد؛ artifact نسخهٔ قبلی تا قبل از آن آخرین بستهٔ تأییدشده باقی می‌ماند.
