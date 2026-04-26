# Spring Tomatoes Example

Готовый демонстрационный сценарий для проекта Greenhouse Climate 3D.

## Идея

Небольшая весенняя теплица для томатов:

- размер теплицы: `10 x 5 x 3` м;
- расчетная сетка: `10 x 5 x 4`;
- длительность: 8 часов;
- шаг времени: 5 минут;
- покрытие: поликарбонат;
- 2 roof-форточки;
- 2 обогревателя;
- 1 увлажнитель;
- 4 контрольные точки растений.

Сценарий показывает холодное утро, солнечный прогрев днем и работу устройств.

## Запуск Simulation

Linux/PuTTY:

```bash
./build/greenhouse3d simulate examples/spring_tomatoes/config.json
```

Windows PowerShell with MinGW build:

```powershell
.\build-mingw\greenhouse3d.exe simulate examples\spring_tomatoes\config.json
```

## Запуск Optimization

```bash
./build/greenhouse3d optimize examples/spring_tomatoes/config.json
```

Windows:

```powershell
.\build-mingw\greenhouse3d.exe optimize examples\spring_tomatoes\config.json
```

## Output

Файлы будут сохранены в:

```text
outputs/examples/spring_tomatoes/
```

Основные файлы:

- `report.txt` - итоговый отчет;
- `summary.json` - краткая сводка;
- `metrics.csv` - метрики по времени;
- `cell_history.csv` - все ячейки 3D-сетки по времени.

## Что смотреть

В `simulate` режиме сравните:

- среднюю температуру у растений;
- ошибку относительно цели `23 C`;
- влажность растений;
- расход энергии.

В `optimize` режиме смотрите рекомендуемые координаты обогревателей в отчете.

Для этого примера базовая расстановка уже подобрана довольно удачно, поэтому оптимизатор может оставить текущие позиции и написать, что они не хуже проверенных вариантов. Это нормальный результат: он показывает, что режим умеет не только менять координаты, но и подтверждать хороший вариант.
