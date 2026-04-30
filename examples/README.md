# Examples

Эта папка содержит готовые сценарии для запуска проекта.

## spring_tomatoes

Весенняя теплица для томатов:

```bash
./build/greenhouse3d simulate examples/spring_tomatoes/config.json
./build/greenhouse3d optimize examples/spring_tomatoes/config.json
```

Windows PowerShell:

```powershell
.\build-mingw\greenhouse3d.exe simulate examples\spring_tomatoes\config.json
.\build-mingw\greenhouse3d.exe optimize examples\spring_tomatoes\config.json
```

Результаты сохраняются в:

```text
outputs/examples/spring_tomatoes/
```

## large_terminal_view

Большая теплица для проверки терминальной карты:

```bash
./build/greenhouse3d simulate examples/large_terminal_view/config.json
```

Windows PowerShell:

```powershell
.\build-mingw\greenhouse3d.exe simulate examples\large_terminal_view\config.json
```

Сценарий использует сетку `100 x 50 x 4`, длительность `43200` секунд и зацикленную анимацию до нажатия `Esc`.

Результаты сохраняются в:

```text
outputs/examples/large_terminal_view/
```
