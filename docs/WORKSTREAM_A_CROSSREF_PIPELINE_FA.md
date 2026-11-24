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

### آزمون runtime برای Batch
- علاوه بر تست dependency-free مبتنی بر AST، `tests/runtime_provider_checks.py` هر دو تابع واقعی `run_batch_writeup` را با executor ساختگی اجرا می‌کند و assert می‌کند `verify_dois_with_crossref=True` به keywordهای `ProcessPoolExecutor.submit` منتقل شده است. هیچ provider یا شبکه‌ای در این آزمون فراخوانی نمی‌شود.

### تکمیل و اعتبارسنجی نهایی این دور
- Python Safety run `45364548308`: Ubuntu و Windows هر دو success.
- Research Runtime run `45364548656`: هر دو job success. Windows Python 3.12 runtime اجرا شد و آزمون runtime جدید Batch در `tests/runtime_provider_checks.py` با پیام `test_crossref_opt_in_reaches_both_batch_process_pools ... ok` و در مجموع 18 تست با نتیجهٔ OK پایان یافت؛ Ubuntu LaTeX/BibTeX واقعی نیز success شد.
- Crossref live run `45363848472`: success؛ درخواست واقعی helper با DOI عمومی `10.1038/nature12373` پاسخ `verified` دریافت کرد.
- Windows build/clean-runner run `45364368515`: هر دو job success. Build و app smoke، ساخت Portable/Installer، استخراج و اجرای Portable، PDF/DOCX در مسیر دارای فاصله/فارسی، نصب silent، اجرای نسخهٔ نصب‌شده، uninstall و حفظ فایل sentinel موفق شدند.
- آخرین HEAD کد محصول در run بسته‌بندی: `0fb187f3fe9a8a85e64cfcc7454b73053bee738a`. تغییرات بعدی در شاخه برای به‌روزرسانی مستندات/دفتر بوده‌اند؛ source code این بسته از همان کد تأییدشده ساخته شده است.
- وضعیت این مسیر: PASS برای اتصال Crossref، مسیر single-run/Batch، retry پس از خطای موقت، Crossref live smoke و گیت‌های CI ذکرشده.
- خارج از این مسیر و همچنان باز: providerهای live با credentials اختصاصی (Gemini/OpenAI/OpenRouter)، آزمایش Microsoft Word 2010 واقعی و اجرای روی دستگاه اصلی کاربر.

### تکمیل ممیزی defensive و smoke-script
- verify_doi_with_crossref اکنون یک کپی از نتیجهٔ cache برمی‌گرداند؛ مصرف‌کننده نمی‌تواند با تغییر دیکشنری، cache سراسری process را خراب کند.
- smoke script شناسهٔ ورودی را با normalize_doi مقایسه می‌کند؛ DOI خام و URL-prefixed هر دو با پاسخ API درست match می‌شوند.
- تست‌های جدید شامل cache immutability، نرمال‌سازی DOI در smoke script و انتقال واقعی گزینه از هر دو batch worker تا فراخوانی write_single_paper هستند.


### تکمیل نهایی مسیر A پس از hardening و بازآزمون‌ها (2026-11-24)
- ممیزی دفاعی helper نشان داد خروجی دیکشنری LRU مستقیم به مصرف‌کننده برمی‌گشت و تغییر آن از سمت caller می‌توانست cache process-local را آلوده کند؛ تابع اکنون یک کپی مستقل برمی‌گرداند. تست regression این رفتار را اثبات می‌کند.
- live smoke script اکنون DOI ورودی را با normalize_doi مقایسه می‌کند؛ DOI خام و DOI دارای پیشوند https://doi.org/ به‌درستی match می‌شوند. تست‌های مستقل mismatch، case-insensitive، status not_found و DOI malformed را بررسی می‌کنند.
- runtime regression جدید هر دو تابع واقعی process_paper_task را اجرا می‌کند و انتقال verify_dois_with_crossref=True را تا فراخوانی write_single_paper assert می‌کند؛ علاوه بر تست قبلی ProcessPoolExecutor.submit.
- Python Safety run 45366337200: Ubuntu و Windows هر دو success؛ هر job 94 تست، شامل cache copy و smoke normalization.
  https://github.com/xperisp2020/paper-orchestra/actions/runs/45366337200
- Research Runtime run 45366337215: Windows Python 3.12 runtime success؛ 19 تست شامل دو تست Batch propagation؛ Ubuntu real LaTeX/BibTeX success.
  https://github.com/xperisp2020/paper-orchestra/actions/runs/45366337215
- Crossref live runs 45366306443 و 45366313174 هر دو success و پاسخ واقعی verified برای DOI عمومی 10.1038/nature12373 ثبت کردند.
  https://github.com/xperisp2020/paper-orchestra/actions/runs/45366306443
  https://github.com/xperisp2020/paper-orchestra/actions/runs/45366313174
- فیلتر workflow ویندوز به scripts/** گسترش یافت؛ چون پوشه scripts به Portable کپی می‌شود، تغییرات این پوشه اکنون بسته‌بندی را trigger می‌کنند.
- Windows build + clean-runner run 45367143582 با هر دو job success پایان یافت؛ Build، app smoke، export package validation، Portable و Installer، اجرای بسته در مسیر فارسی/دارای فاصله، install/uninstall و حفظ داده sentinel موفق شدند.
  https://github.com/xperisp2020/paper-orchestra/actions/runs/45367143582
- Portable: artifact 14906139648؛ size 27093368 bytes؛ ZIP SHA-256 dd4a8991fde4866163d545cf8339224c07739a920ebe6f2c8ab1b511399a45cb.
- Installer: artifact 14907232374؛ size 19931308 bytes؛ ZIP SHA-256 57dd5238c287f47dd431676e1fa19537c80ef98c7b5851ce079860400b05b65a.
- انقضای هر دو artifact: 2027-02-22T09:38:40Z. این مقادیر hash آرشیو ZIP هستند.
- نتیجه: Workstream A در سطح پیاده‌سازی، helper/live smoke، Single-run، Batch، CLI/Qt، Python Safety، Research Runtime و Windows clean-runner PASS است. موارد خارج از این مسیر همچنان Gemini/OpenAI/OpenRouter live با credentials اختصاصی، Word 2010 واقعی و دستگاه اصلی کاربر است.
