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

## asymmetric_12x6

Несимметричный сценарий на стандартной сетке `12 x 6 x 4`:

```bash
./build/greenhouse3d simulate examples/asymmetric_12x6/config.json
```

Windows PowerShell:

```powershell
.\build-mingw\greenhouse3d.exe simulate examples\asymmetric_12x6\config.json
```

В этом примере один обогреватель сильный и смещен влево, второй слабее и смещен вправо вверх, а форточки открыты по-разному. Его удобно запускать, чтобы увидеть неравномерную температуру на терминальной карте.

Результаты сохраняются в:

```text
outputs/examples/asymmetric_12x6/
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

Сценарий использует расчетную сетку `100 x 50 x 4`, длительность `43200` секунд и зацикленную анимацию до нажатия `Esc`. Для PuTTY карта прорежена до компактного вида через `display_stride_x` и `display_stride_y`.

Результаты сохраняются в:

```text
outputs/examples/large_terminal_view/
```
