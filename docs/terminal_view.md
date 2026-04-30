# Terminal View

Терминальная визуализация работает в режиме `simulate` и показывает анимацию одного слоя 3D-сетки теплицы.

Запуск:

```bash
./build/greenhouse3d simulate configs/default_config.json
```

На Windows PowerShell с MinGW-сборкой:

```powershell
.\build-mingw\greenhouse3d.exe simulate configs\default_config.json
```

## Настройки

Блок находится внутри `output`:

```json
"terminal_view": {
  "enabled": true,
  "field": "temperature",
  "layer_z": 0,
  "frame_interval_seconds": 600,
  "sleep_ms": 150,
  "use_colors": true,
  "clear_screen": true,
  "loop_playback": false,
  "show_devices": true,
  "project_devices_to_layer": true,
  "auto_scale": false,
  "fixed_min_value": 0.0,
  "fixed_max_value": 40.0
}
```

## Поле карты

`field` выбирает отображаемую величину:

- `temperature` - температура ячеек.
- `humidity` - влажность ячеек.
- `temperature_error` - отклонение температуры ячейки от средней целевой температуры растений.

Для температуры удобно использовать диапазон `0..40`, для влажности `0..100`, для ошибки температуры `0..10`.

## Слои

`layer_z` выбирает слой по высоте:

- `0` - нижний слой, ближе к растениям и обогревателям.
- `1` или `2` - средние слои.
- `3` - верхний слой для сетки `nz = 4`, ближе к крыше и форточкам.

Если устройство находится на другом слое, параметр `project_devices_to_layer: true` все равно покажет его на карте по координатам `x/y`. Это удобно для вида сверху.

## Символы

Значения ячеек переводятся в шкалу:

```text
low  . : - = + * # % @  high
```

Устройства печатаются поверх карты:

```text
P plant
H heater
V vent
M humidifier
* multiple devices
```

## PuTTY

Если цвета отображаются неправильно:

```json
"use_colors": false
```

Если нужно увидеть все кадры подряд, а не анимацию на одном месте:

```json
"clear_screen": false
```

Если нужно зациклить анимацию до нажатия `Esc`:

```json
"loop_playback": true
```

В интерактивном терминале PuTTY клавиша `Esc` останавливает анимацию без нажатия Enter. После этого программа продолжает обычный сценарий: экспортирует файлы и печатает отчет.

Скорость анимации:

- `frame_interval_seconds` - какой шаг симуляционного времени показывать.
- `sleep_ms` - реальная задержка между кадрами в терминале.

## Большая сетка

Для проверки крупной карты есть отдельный пример:

```bash
./build/greenhouse3d simulate examples/large_terminal_view/config.json
```

В нем сетка `100 x 50 x 4`, длительность `43200` секунд, а `loop_playback` включен. CSV в этом примере отключен, чтобы не создавать огромный файл истории всех ячеек.

## Архитектура

`TerminalView.cpp` только читает готовый `SimulationResult`. Он не меняет физику, сетку, устройства и экспорт.

Правильная цепочка в `simulate`:

```text
Simulator -> TerminalView -> Exporter -> Report
```
