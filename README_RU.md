# GUITAR TUNE PRO — проект VST3 для Windows 11

Это исходный проект первой версии гитарного тюнера для FL Studio. Плагин анализирует входящий звук и пропускает аудио без изменения.

## Возможности
- VST3 x64 на C++ / JUCE 8.0.6.
- Режимы AUTO, MANUAL и CHROMATIC.
- 12 строев: E Standard, Eb Standard, D Standard, Drop D/C/B/A, C Standard, Open G/D/E, DADGAD.
- Эталон A4: 432–445 Hz.
- Отображение ноты, частоты и отклонения в центах.
- Сохранение параметров проекта.

## Важно о первой версии
Исходный проект ещё не собран и не протестирован в FL Studio на твоём компьютере. Детектор рассчитан на настройку одной открытой струны за раз: приглушай остальные струны. AUTO выбирает ближайшую целевую ноту из выбранного строя, но не разделяет одновременно звучащие струны. Значение с десятыми долями цента — формат отображения, а не обещание такой физической точности. Нет Noise Gate и String Lock, как и договаривались.

## Сборка
1. Установи в Visual Studio workload **Desktop development with C++**, включая MSVC v143 и Windows 10/11 SDK.
2. Убедись, что установлен Git и CMake.
3. Распакуй ZIP в простую папку, например `C:\\GuitarTunePro`.
4. Открой **Developer PowerShell for VS 2022**.
5. Перейди в папку проекта (путь замени на свой):

```powershell
cd C:\\GuitarTunePro
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

На первой конфигурации CMake скачает JUCE 8.0.6 с GitHub, поэтому нужен интернет. Если CMake 4.5.0-rc2 выдаст ошибку совместимости, установи стабильный релиз CMake с сайта cmake.org.

6. После успешной сборки найди `GuitarTunePro.vst3` в `build\\GuitarTunePro_artefacts\\Release\\VST3\\`.
7. Скопируй всю папку `GuitarTunePro.vst3` в `C:\\Program Files\\Common Files\\VST3`.
8. В FL Studio открой Plugin Manager, нажми Find installed plugins и загрузи Guitar Tune Pro на Mixer Insert.

## Arturia MiniFuse
Подключи гитару в инструментальный вход MiniFuse, выбери драйвер Arturia MiniFuse ASIO в FL Studio, назначь соответствующий вход на Mixer Insert и вставь тюнер в FX-слот этого Insert. Для настройки играй по одной открытой струне.

## Поддержать проект

Если GUITAR TUNE PRO X оказался полезен и ты хочешь поддержать его дальнейшее развитие, можно отправить донат:

- **Криптовалюта:** USDT
- **Сеть:** TRON (TRC20)
- **Адрес кошелька:** `TBUeb52YrGx1fjmJMhVCZnnYBup3nyYKhf`

**Важно:** отправляй USDT только через сеть TRON (TRC20). Не отправляй другие монеты или через другие сети — средства могут быть безвозвратно потеряны.

Спасибо за поддержку независимой разработки!
