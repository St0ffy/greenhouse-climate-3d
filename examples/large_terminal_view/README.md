# Large Terminal View Example

Большой сценарий для проверки терминальной визуализации на сетке `100 x 50 x 4`.

Особенности:

- размер теплицы: `100 x 50 x 4` метров;
- расчетная сетка: `100 x 50 x 4`, всего `20000` ячеек;
- длительность: `43200` секунд, то есть 12 часов;
- шаг симуляции: `300` секунд;
- терминальный слой: `z = 0`;
- анимация зациклена через `loop_playback: true`;
- выход из анимации: клавиша `Esc`;
- CSV отключен, чтобы не создавать очень большой `cell_history.csv`.

Запуск:

```bash
./build/greenhouse3d simulate examples/large_terminal_view/config.json
```

На Windows PowerShell:

```powershell
.\build-mingw\greenhouse3d.exe simulate examples\large_terminal_view\config.json
```

После нажатия `Esc` программа выйдет из анимации, сохранит `summary.json` и `report.txt` в:

```text
outputs/examples/large_terminal_view/
```

Если нужно сохранить полный CSV по всем ячейкам, можно включить:

```json
"write_csv": true
```

Но для этой сетки файл будет очень большим, потому что одна строка пишется для каждой ячейки в каждом кадре.
