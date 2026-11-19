# Murmulator — ZX Spectrum

Эмулятор ZX Spectrum для Murmulator и Murmulator 2 на RP2040 и RP2350 (ARM). В прошивке есть файловый менеджер, поддержка TR-DOS, образов лент TAP, снимков Z80/SNA и звука AY, TurboSound и Soundrive.

[Прошивки форка](https://github.com/DnCraptor/Murmulator_rp2040/releases) · [Группа проекта](https://t.me/ZX_MURMULATOR) · [Исходный проект](https://github.com/MadedCat/Murmulator_rp2040)

## Выбор прошивки

Имена файлов имеют вид `murmulator-<плата>-<видео>-v<версия>.uf2`.

| Код платы | Разводка | Процессор | Параметры CMake |
| --- | --- | --- | --- |
| `m1p1` | Murmulator | RP2040 | `-DPICO_BOARD=murmulator -DPICO_PLATFORM=rp2040` |
| `m1p2` | Murmulator | RP2350 (ARM) | `-DPICO_BOARD=murmulator -DPICO_PLATFORM=rp2350` |
| `m2p1` | Murmulator 2 | RP2040 | `-DPICO_BOARD=murmulator2 -DPICO_PLATFORM=rp2040` |
| `m2p2` | Murmulator 2 | RP2350 (ARM) | `-DPICO_BOARD=murmulator2 -DPICO_PLATFORM=rp2350` |

Для каждой платы предусмотрены три сборки.

| `MURM_VIDEO` | Видеовыход |
| --- | --- |
| `vga-hdmi-tft` | VGA, HDMI или TFT в одной прошивке. TFT: ST7789, ST7789V, ILI9341, ILI9341V, GC9A01. |
| `composite` | Композитный выход с внешним видеокодером CXA1645M, режимы NTSC и PAL. |
| `soft-composite` | Программный композитный выход NTSC/PAL, требует соответствующей схемы ЦАП. |

Подключите плату в режиме BOOTSEL и скопируйте выбранный UF2 на появившийся USB-накопитель. После перезагрузки откройте файловый менеджер клавишей Win или Home, меню быстрых настроек — F12 или Print Screen, справку — F1. Настройки хранятся в `/murmulator.cfg` на SD-карте. При переходе между видеобэкендами сохраните копию файла настроек и выберите видеорежим заново, поскольку значения `video_out` зависят от сборки.

[Справка на русском](release/help_murmulator_rus.txt) · [English help](release/help_murmulator_eng.txt) · [Release notes](RELEASE_NOTES.md)

## Сборка

Понадобятся Pico SDK 2.2.0 с подмодулем TinyUSB, ARM GNU Toolchain (проверено с GCC 14.2.1), CMake, Ninja и компилятор C/C++ для сборки инструментов SDK. Для архива всей матрицы также нужны Bash, `zip` и `sha256sum`.

```sh
git clone --branch 2.2.0 --depth 1 https://github.com/raspberrypi/pico-sdk.git
git -C pico-sdk submodule update --init lib/tinyusb
export PICO_SDK_PATH="$PWD/pico-sdk"

git clone --branch main https://github.com/DnCraptor/Murmulator_rp2040.git
cd Murmulator_rp2040
cmake -S . -B build/m1p1-vga -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DPICO_BOARD=murmulator -DPICO_PLATFORM=rp2040 -DMURM_VIDEO=vga-hdmi-tft
cmake --build build/m1p1-vga
```

UF2 появляется в каталоге сборки. Без явного выбора CMake собирает `m2p2-vga-hdmi-tft`. Используйте отдельный каталог сборки для каждого сочетания платы, процессора и видео. SDK использует установленный picotool 2.2.0 или загружает его при сборке.

Для сборки всех 12 UF2 запустите `bash scripts/build-release.sh`. Архив с прошивками, контрольными суммами SHA-256 и справкой появляется в `build/release/`. GitHub Actions собирает такой же архив при обновлении `main`. Кнопка ручного запуска появится, когда `main` станет основной веткой. Для публикации приложите архив из успешной сборки к релизу соответствующего тега и используйте [RELEASE_NOTES.md](RELEASE_NOTES.md) как текст релиза.

## Авторы

- Автор ядра и железа: Алекс Екб(@Alex_Eburg)
- Поддержка TR-DOS: First Last (@First_0x26_Last), Кошкин Хвост (@tecnocat)
- Файловый менеджер: Алекс Екб(@Alex_Eburg), Кошкин Хвост(@tecnocat)
- Расширение SoftTurboSound: Алекс Екб(@Alex_Eburg), Кошкин Хвост(@tecnocat), Sa Gin(@sa gin)
- Расширение TurboSound: Алекс Екб(@Alex_Eburg), Sa Gin(@sa gin), Кошкин Хвост(@tecnocat)
- Расширение Soundrive: Sa Gin(@sa gin), Кошкин Хвост(@tecnocat)
- TFT драйвер: Алекс Екб(@Alex_Eburg), Кошкин Хвост(@tecnocat)

## English quick start

Choose one of the 12 UF2 files from the [fork releases](https://github.com/DnCraptor/Murmulator_rp2040/releases). Match the board and processor in the first table and choose a video backend in the second table. Copy the UF2 to the drive that appears when you connect the board in BOOTSEL mode. Use Win or Home for the file manager, F12 or Print Screen for the quick menu, and F1 for help. Back up `/murmulator.cfg` before switching video backends, since each backend interprets `video_out` differently.

The build example above uses Pico SDK 2.2.0 and Arm GNU Toolchain. Run `bash scripts/build-release.sh` to package all 12 builds, SHA-256 checksums and both help files. The build workflow uploads the package for each push to `main`. The manual run button becomes available when `main` is the default branch.
