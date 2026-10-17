# Promptlar tahlili — topilgan xato va kamchiliklar

> Egasi uchun (o'zbekcha). Asl ikki prompt (Master prompt va Dizayn prompt)
> sinchiklab ko'rib chiqildi. Tuzatilgan versiyalar:
> `docs/MASTER_PROMPT.md` va `docs/DESIGN_PROMPT.md`.

## Umumiy xulosa

Promptlar juda yaxshi yozilgan: halollik, xavfsizlik, original yadro,
bosqichma-bosqich ish — hammasi to'g'ri. Lekin ular **kompyuteri bor
dasturchi** uchun yozilgan va bir nechta qarama-qarshilik bor. Claude Code
ularni bir martada "hammasini qil" deb tushunib, keragidan katta va
tekshirilmagan kod yozib yuborishi mumkin edi. Quyidagilar tuzatildi.

## Topilgan muammolar va tuzatishlar

| # | Muammo | Nima uchun xavfli | Tuzatish |
|---|---|---|---|
| 1 | Sizda kompyuter yo'q, prompt esa "host kompyuter, real hardware" deb faraz qiladi | Claude test qila olmaydigan narsani "ishlaydi" deyishi mumkin | GitHub Actions CI — asosiy haqiqat manbai. Hammasi QEMU emulatorida avtomatik test qilinadi |
| 2 | Dizayn prompt "avval Dizayn tizimini boshla" deydi, Master prompt "avval yadro" deydi | Claude qaysi biridan boshlashni bilmaydi | Tartib aniq: avval yadro yuklanadi (Phase 2), keyin Dizayn spec (Phase 6). Desktop faqat displey+klaviatura+IPC tayyor bo'lgach (Phase 18+) |
| 3 | 17-bo'limdagi 10 qadam bitta buyruqda bajarilishi kerakdek | Juda katta, tekshirilmagan ish | 3 fazaga bo'lindi (Phase 0, 1, 2). Bir buyruq = bir faza |
| 4 | "Git commit faqat ruxsat bo'lsa" — ruxsat qachon berilishi aytilmagan | Claude har safar ruxsat so'rab to'xtab qoladi | Har faza oxirida `main`ga commit + push ruxsat etilgan. Force-push taqiqlangan |
| 5 | Litsenziya tanlash "egasidan so'ra" — lekin qachon? | Loyiha noaniq qoladi | Phase 0 da taqqoslash jadvali bilan so'raladi. Javobgacha "license pending" |
| 6 | Linux/Windows moslik maqsadi noaniq ("Linux ilovalari ishlasin") | Hech qachon "tugadi" deb bo'lmaydi | Aniq birinchi maqsadlar: musl bilan statik Linux dasturi (6 ta syscall) va MinGW Windows konsol dasturi (3 ta funksiya) |
| 7 | Wine/ReactOS haqida "o'rganish mumkin" deyilgan | Ularning kodi GPL/LGPL — nusxa olsangiz loyiha litsenziyasi majburan o'zgaradi | Ulardan kod nusxalash qat'iyan taqiqlangan; faqat ochiq hujjatlar |
| 8 | Shrift litsenziyasi umuman aytilmagan | Segoe UI / San Francisco ishlatilsa — qonun buzilishi | Faqat SIL OFL shriftlar yoki o'zimizniki; `LICENSES.md`da qayd |
| 9 | Grafika qayerda ishlashi aniq emas | Desktop yadro ichiga tushib qolsa — xavfsizlik va barqarorlik yomon | Grafika user-space'da (oddiy dastur sifatida). Yadro faqat xotira, kirish hodisalari, IPC beradi |
| 10 | "Placeholder ikonka bo'lmasin" va "bosqichma-bosqich" qarama-qarshi | Dastlab hamma ikonka tayyor bo'lmaydi | Ikonka manifesti: `draft` / `final`. Relizda `draft` bo'lsa test yiqiladi |
| 11 | Kontrast "accessible" deyilgan, lekin raqam yo'q | Tekshirib bo'lmaydi | WCAG 2.2 AA (4.5:1), yuqori kontrast rejimi 7:1 — avtomatik test |
| 12 | Blur/animatsiya — GPU yo'q paytda | Dastlab GPU drayver yo'q, hammasi dasturiy chiziladi | Har bir effekt dasturiy renderda 60 Hz da ishlashi shart; blur standart o'chiq |
| 13 | Benchmark CI emulatorida | Emulator vaqtlari aniq emas | Faqat bir xil muhitdagi natijalar solishtiriladi |
| 14 | Klaviatura tartiblari | O'zbek foydalanuvchi uchun muhim | O'zbek lotin va kirill tartiblari, interfeys tillari: ingliz + o'zbek |
| 15 | "SMY-OS" nomi | Savdo belgisi to'qnashuvi bo'lishi mumkin | Ochiq savol sifatida `LICENSES.md`da; ishni to'xtatmaydi |
| 16 | Hujjatlar ro'yxati ROADMAP.md ni ildizda va docs/ ichida ham nazarda tutadi | Ikki nusxa chalkashlik | `ROADMAP.md` — ildizda, bitta |
| 17 | Rust "ko'rib chiq" deyilgan | Boshida ikki tilni aralashtirish ishni sekinlashtiradi | Avval faqat C17 + assembler; Rust faqat ADR bilan, aniq komponent uchun |
| 18 | Xavfsizlik "keyinroq" bo'lib qolishi mumkin | Yadro boshidanoq himoyasiz qurilsa, keyin tuzatish qiyin | Birinchi fazalardanoq W^X, NX, SMEP/SMAP, stack protector |

## Qanday ishlatiladi

1. Shu repoda yangi Claude Code sessiyasini oching.
2. Faqat **"davom et"** deb yozing.
3. Claude `ROADMAP.md`dagi birinchi tugallanmagan fazani bajaradi, CI ni
   tekshiradi, o'zbekcha qisqa hisobot beradi va to'xtaydi.
4. Xato bo'lsa: **"xato: ..."** deb yozing. Holatni bilish uchun: **"holat"**.

## Haqiqatga to'g'ri baho

Bu — yillar davom etadigan loyiha. Linux va Windows yadrolari ustida minglab
odamlar o'nlab yil ishlagan. Roadmapdagi 39 faza (0–38) "tayyor OS" emas, balki
**ishlaydigan poydevor**: o'z yadrosi, oddiy desktop, birinchi Linux va
Windows dasturlari. "Har qanday Windows o'yini ishlaydi" kabi natija bu
rejaga kirmaydi va hech kim bunga va'da bera olmaydi.

---

## Installer prompt (o'rnatish tizimi) — tahlil

Tuzatilgan versiya: `docs/INSTALLER_PROMPT.md`. Roadmapga 27–37 fazalar
qo'shildi, "eng kichik bootable ISO" talabi Phase 2 ga birlashtirildi.

| # | Muammo | Nima uchun xavfli | Tuzatish |
|---|---|---|---|
| 1 | "ISO ichida grafik muhit, drayverlar..." — hali yo'q narsalar | Claude soxta bo'laklar qo'shishi mumkin | ISO faqat mavjud narsadan yig'iladi va har fazada o'sadi |
| 2 | USB ga yozilganda ham yuklanishi aytilmagan | Oddiy ISO USB'dan yuklanmasligi mumkin | "Hybrid" ISO talabi; CI da CD va disk sifatida ikkalasi test qilinadi |
| 3 | Real kompyuter drayverlari yo'q (QEMU'da virtio bor, haqiqiy PC'da yo'q) | Haqiqiy kompyuterda disk ham, klaviatura ham ko'rinmaydi | Yangi fazalar: AHCI (SATA), NVMe, USB (xHCI) klaviatura/sichqoncha |
| 4 | Diskka yozish (GPT, FAT32) rejada yo'q edi — faqat o'qish bor edi | O'rnatib bo'lmaydi | Phase 29: GPT yozish, FAT32 yozish (EFI bo'limi uchun), asosiy fayl tizimi |
| 5 | Live rejimda ichki diskni o'zgartirmaslik faqat "va'da" | UI xatosi diskni buzishi mumkin | Saqlash qatlamida majburiy read-only; test disk xeshi o'zgarmaganini tekshiradi |
| 6 | Dual-boot uchun Windows bo'limini kichraytirish nazarda tutilgan | NTFS ni kichraytirish — ma'lumot yo'qotishning eng katta sababi | SMY-OS begona bo'limlarni o'zgartirmaydi. Faqat foydalanuvchi oldindan bo'shatgan joyga o'rnatadi. BitLocker va Fast Startup ogohlantirishi |
| 7 | UEFI boot yozuvi — umumiy EFI bo'limida | `\EFI\BOOT\BOOTX64.EFI` ni almashtirish Windowsni yuklanmaydigan qilib qo'yadi | Bu fayl faqat SMY-OS uchun ajratilgan diskda yoziladi. Boot yozuvi eng oxirida yoziladi |
| 8 | Uzilgan o'rnatish | Tok o'chsa yarim o'rnatilgan tizim yuklanadi | Qadamlar jurnali; boot yozuvi oxirgi qadam; keyingi yuklanishda aniqlanadi |
| 9 | Tasdiqlash shunchaki "Next" tugmasi bo'lishi mumkin | Noto'g'ri disk o'chib ketadi | Tasdiq aniq rejaga va disk identifikatoriga (model+serial+hajm) bog'langan; disk nomini kiritish |
| 10 | Grafik installer darhol | UI toolkit hali yo'q, test qilib bo'lmaydi | Avval installer "dvigateli" + matnli interfeys (avtomatik test uchun), GUI keyin xuddi shu dvigatel ustida |
| 11 | Secure Boot | Limine Microsoft imzosiga ega emas; ko'p kompyuterlarda yuklanmaydi | Hujjatda ochiq aytiladi: Secure Boot o'chirilishi kerak. Imzolash — alohida faza (37) |
| 12 | Akkaunt paroli | Noto'g'ri saqlash — xavfsizlik teshigi | Argon2id, tayyor kutubxona orqali |
| 13 | Avtomatik testlar real diskka tegishi mumkin | Ma'lumot yo'qolishi | Test runner haqiqiy qurilma yo'lini rad etadi; faqat image fayllar |
| 14 | **Sizda kompyuter yo'q** | USB yozish va haqiqiy PCda sinash uchun kompyuter kerak | Phase 36 — OWNER DECISION: sinov kompyuteri (diski o'chirilsa bo'ladigan) va USB yozish uchun boshqa kompyuter kerak bo'ladi |
| 15 | Reproducible build qanday tekshiriladi — aytilmagan | "Reproducible" deb aytish oson, isbotlash qiyin | CI ikki marta yig'adi va SHA-256 ni solishtiradi |
