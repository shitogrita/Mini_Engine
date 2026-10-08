# Mini Engine — техническая спецификация

> **Проект:** Mini Engine  
> **Тип:** интерактивный 3D-движок и редактор сцены  
> **Язык:** C++20  
> **UI:** Qt 6 / Widgets / `QOpenGLWidget`  
> **Graphics API:** OpenGL 3.3 Core  
> **Build:** CMake / Ninja  
> **Фокус проекта:** многопоточная система CPU-обновления интерактивной 3D-сцены  
> **Статус:** активная разработка

---

## 1. Назначение документа

Этот документ фиксирует техническую архитектуру Mini Engine, требования к подсистемам, правила многопоточности, устройство rendering pipeline, редактора и benchmark-системы.

README отвечает на вопрос «что умеет проект и как его запустить», а данная спецификация отвечает на вопросы:

- как устроен движок;
- какие подсистемы входят в него;
- кто за что отвечает;
- какие данные принадлежат CPU Update, а какие Renderer;
- где разрешена многопоточность;
- где запрещены OpenGL-вызовы;
- как синхронизируются worker-потоки;
- как выполняется один кадр;
- как устроены тени, освещение и viewport;
- как проводится benchmark;
- какие ограничения текущей реализации известны заранее;
- какие критерии определяют корректность реализации.

Документ предназначен для разработки, сопровождения проекта и использования в пояснительной записке ВКР.

---

## 2. Назначение Mini Engine

Mini Engine — собственный учебный 3D-движок с редактором сцены, предназначенный для интерактивной визуализации, изучения архитектуры rendering pipeline и исследования многопоточного CPU-обновления объектов.

Система включает две основные части:

```text
Mini Engine
├── Editor
│   ├── EditorWindow
│   ├── SceneViewport
│   ├── Hierarchy
│   ├── Inspector
│   ├── Project
│   └── Tests / Benchmark
│
└── Engine Core
    ├── Scene
    ├── SceneObject
    ├── Transform
    ├── MotionState
    ├── Camera
    ├── SceneUpdateSystem
    ├── ThreadPool
    ├── Renderer
    ├── Mesh / Shader / Texture / Material
    ├── PointLight / PointShadowMap
    ├── ModelImporter / ObjParser
    └── SceneSerializer
```

---

## 3. Цели проекта

Основная инженерная цель — реализовать минимальный, но архитектурно полноценный 3D-движок, в котором основные подсистемы не скрыты за сторонним engine framework.

Проект должен поддерживать:

- создание сцены;
- создание примитивов;
- импорт OBJ-моделей;
- камеру;
- Perspective и Orthographic projection;
- материалы и текстуры;
- несколько Point Light;
- Point Shadow Mapping;
- выбор объектов;
- Transform gizmo;
- сохранение и загрузку сцены;
- однопоточное CPU-обновление;
- многопоточное CPU-обновление;
- воспроизводимый benchmark.

Исследовательская цель проекта — сравнить две стратегии исполнения одной и той же логики обновления сцены - SingleThreaded,
MultiThreaded


Ключевой принцип эксперимента:

> В Single Thread и Multi Thread должна выполняться одинаковая логика `UpdateObject()`. Меняется только способ исполнения.

---

## 4. Технологический стек

| Область | Технология |
|---|---|
| Язык | C++20 |
| Editor UI | Qt 6 Widgets |
| Viewport | `QOpenGLWidget` |
| Graphics API | OpenGL 3.3 Core |
| Shaders | GLSL |
| OpenGL Loader | GLAD |
| Build | CMake |
| Build backend | Ninja |
| Multithreading | `std::thread` |
| Synchronization | `std::mutex`, `std::condition_variable` |
| Task representation | `std::function<void()>` |
| Timing | `std::chrono::steady_clock` |
| Models | Wavefront OBJ |
| Image loading | `stb_image` |
| Scene format | собственный `.scene` |

---

## 5. Архитектура кадра

Один кадр должен проходить через строго определённые стадии:

```text
Input
  ↓
Scene CPU Update
  ↓
Synchronization / WaitIdle
  ↓
Point Shadow Pass
  ↓
Main Color Pass
  ↓
Editor Helpers
  ↓
Frame complete
```

Важнейшее правило:

> Renderer не должен читать Transform объекта одновременно с тем, как worker thread изменяет этот Transform.

Поэтому Multi Thread Update всегда заканчивается `WaitIdle()` до начала Render Pass.

---

## 6. Разделение ответственности CPU и GPU

### CPU / worker threads

Worker threads могут выполнять:

- изменение `Transform`;
- обработку `MotionState`;
- независимые CPU-вычисления;
- подготовку данных, не требующих OpenGL Context.

### Main Qt/OpenGL thread

Только основной OpenGL-поток выполняет:

- `gl*` вызовы;
- binding VAO/VBO/EBO;
- shader activation;
- texture binding;
- framebuffer operations;
- draw calls;
- shadow map rendering;
- final color rendering.

OpenGL-вызовы в worker threads в текущей архитектуре запрещены.

---

## 7. Scene

`Scene` является контейнером объектов сцены.

Основные обязанности:

- хранение `SceneObject`;
- добавление объектов;
- очистка;
- доступ к объектам;
- участие в save/load;
- предоставление данных системам Update и Render.

`Scene` не должна сама выполнять OpenGL rendering и не должна владеть ThreadPool.

---

## 8. SceneObject

`SceneObject` — базовая сущность мира.

Объект может представлять:

- Cube;
- Plane;
- Sphere;
- Imported Model;
- Point Light;
- Empty object.

Типичная структура:

```text
SceneObject
├── name
├── type
├── Transform
├── BoundingBox
├── Mesh / render data
├── Material
├── MotionState
└── PointLight (optional)
```

Не каждый объект обязан иметь Mesh.

---

## 9. Transform

`Transform` хранит пространственное состояние объекта:

```text
position
rotation
scale
```

Он используется одновременно:

- Scene Update;
- Renderer;
- Gizmo;
- Inspector;
- Picking;
- Frame Selected;
- Serialization.

Основной инвариант:

> Все подсистемы должны работать с одним фактическим Transform объекта, а не с независимыми копиями.

---

## 10. MotionState

`MotionState` описывает минимальную динамику объекта:

```cpp
struct MotionState {
    Vec3 linear_velocity{0.0f, 0.0f, 0.0f};
    Vec3 angular_velocity{0.0f, 0.0f, 0.0f};
    bool enabled = false;
};
```

Если `enabled == false`, объект пропускается системой CPU Update.

Если включён:

```text
position += linear_velocity * delta_time
rotation += angular_velocity * delta_time
```

---

## 11. SceneUpdateSystem

`SceneUpdateSystem` отвечает только за CPU-обновление состояния Scene.

Основной интерфейс:

```cpp
void Update(Scene& scene, float delta_time, ExecutionMode mode);
```

Маршрутизация:

```text
Update()
  ↓
ExecutionMode
  ├── SingleThreaded → UpdateSingleThreaded()
  └── MultiThreaded  → UpdateMultiThreaded()
```

---

## 12. Общая UpdateObject

Оба execution mode обязаны использовать одну функцию:

```text
                  UpdateObject()
                      |
              +-------+-------+
              |               |
         Single Thread    Multi Thread
```

Это необходимо для корректности benchmark.

Если Single и Multi будут использовать разные формулы, эксперимент перестанет измерять только эффект параллелизма.

---

## 13. Single Thread mode

Последовательный baseline:

```text
for object in scene:
    if object is null:
        continue

    UpdateObject(object)
```

Требования:

- используется один поток;
- ThreadPool jobs не создаются;
- используется та же `UpdateObject()`;
- режим является контрольной точкой benchmark.

---

## 14. Multi Thread mode

Multi Thread использует persistent ThreadPool.

Пример разбиения:

```text
50 000 объектов
10 workers

Job 0 → [0, 5000)
Job 1 → [5000, 10000)
...
Job 9 → [45000, 50000)
```

Каждая job обрабатывает диапазон объектов обычным последовательным циклом.

---

## 15. Почему не используется одна job на объект

Плохая модель:

```text
50 000 objects
→ 50 000 Enqueue()
```

Это создаёт слишком большой overhead:

- mutex contention;
- очередь;
- task wrapping;
- scheduling;
- notifications;
- лишние wakeups.

Поэтому используется coarse-grained chunking.

---

## 16. Расчёт количества jobs

Количество jobs ограничено числом worker-потоков и числом объектов:

```text
job_count = min(worker_count, object_count)
```

Размер chunk:

```cpp
const std::size_t chunk_size =
    (object_count + job_count - 1) / job_count;
```

Это деление с округлением вверх.

Граница:

```cpp
end = std::min(begin + chunk_size, object_count);
```

Диапазон:

```text
[begin, end)
```

---

## 17. ThreadPool

ThreadPool реализован самостоятельно и предназначен для повторного использования потоков между кадрами.

Основные поля:

```text
workers_
tasks_
mutex_
task_condition_
idle_condition_
stopping_
active_workers_
```

---

## 18. Worker lifecycle

При создании ThreadPool создаются worker threads.

Каждый worker выполняет:

```text
WorkerLoop
  ↓
wait(task_condition)
  ↓
take task
  ↓
execute task outside mutex
  ↓
update active_workers
  ↓
wait again
```

Потоки не создаются заново каждый кадр.

---

## 19. Очередь задач

Очередь:

```cpp
std::queue<std::function<void()>> tasks_;
```

`Enqueue()`:

```text
lock
push task
unlock
notify_one
```

Нельзя выполнять `task()` под общим `mutex_`, иначе workers будут фактически сериализованы.

---

## 20. Condition variables

Используются две condition variables.

### `task_condition_`

Сигнал от producer/main thread к workers.

Назначение:

- новая задача;
- начало shutdown.

### `idle_condition_`

Сигнал от workers к тому, кто вызвал `WaitIdle()`.

Назначение:

- уведомить, что пул, возможно, полностью завершил работу.

---

## 21. Понятие idle

Правильное условие:

```cpp
tasks_.empty() && active_workers_ == 0
```

Недостаточно проверять только `tasks_.empty()`.

Причина:

```text
worker забрал последнюю задачу
↓
queue уже empty
↓
worker ещё выполняет работу
```

---

## 22. WaitIdle

`WaitIdle()` является барьером между Update и Render.

```text
Enqueue jobs
↓
Workers execute
↓
WaitIdle
↓
all Transform updates complete
↓
Render
```

Это основная защита от чтения Renderer незавершённых Transform.

---

## 23. Shutdown ThreadPool

Destructor:

```text
lock
stopping = true
unlock
notify_all
join all workers
```

Worker завершает loop только при:

```cpp
stopping_ && tasks_.empty()
```

Это позволяет корректно завершить уже поставленные задачи.

---

## 24. Правила thread safety

MT = MultiThreading

### MT-01

Один SceneObject не должен одновременно изменяться двумя jobs.

### MT-02

Renderer не читает Transform до завершения `WaitIdle()`.

### MT-03

OpenGL запрещён в worker threads.

### MT-04

`scene.GetObjects()` не должен структурно изменяться во время parallel update.

### MT-05

`begin`, `end`, `delta_time` захватываются lambda по значению.

### MT-06

`objects` может захватываться по ссылке только при гарантии, что контейнер жив и не меняется до `WaitIdle()`.

---

## 25. Renderer

Renderer инкапсулирует основные OpenGL operations.

Он отвечает за:

- Initialize;
- frame begin;
- framebuffer binding;
- viewport;
- mesh draw;
- line draw;
- depth draw;
- взаимодействие с Shader.

Renderer не отвечает за Scene logic.

---

## 26. Mesh

Mesh хранит GPU-ready геометрию.

Типичный набор OpenGL resources:

```text
VAO
VBO
EBO
```

Mesh может быть создан:

- из PrimitiveGenerator;
- из imported OBJ data.

---

## 27. Shared Mesh в stress tests

Stress test не должен создавать новый Mesh для каждого Cube.

Правильно:

```text
SceneObject 0 ─┐
SceneObject 1 ─┤
SceneObject 2 ─┼──> one shared Cube Mesh
...           ─┤
SceneObject N ─┘
```

Это позволяет benchmark измерять число SceneObjects и Update workload, а не создание одинаковых GPU resources.

---

## 28. Shader system

Основные shader pairs:

```text
basic.vert / basic.frag
lamp.vert / lamp.frag
pointShadow.vert / pointShadow.frag
```

Shader abstraction отвечает за:

- загрузку source;
- compilation;
- linking;
- `Use()`;
- uniforms.

Critical shader file missing должен приводить к понятной ошибке, а не к тихому отключению функциональности.

---

## 29. Материалы и текстуры

Material задаёт визуальные свойства поверхности.

Текущая система должна поддерживать:

- цвет;
- diffuse texture;
- параметры освещения, необходимые shader.

Texture2D является GPU resource и должен создаваться/уничтожаться при валидном OpenGL Context.

---

## 30. Point Light

Point Light представлен через SceneObject.

Position источника света берётся из:

```text
SceneObject.Transform.position
```

Параметры:

- color;
- intensity;
- enabled.

Такой подход позволяет использовать обычный Inspector и Transform для источника света.

---

## 31. Несколько источников света

Basic shader получает только включённые Point Light.

Существует максимальное число источников текущего shader pipeline:

```text
kMaxPointLights
```

Если источников больше:

- они остаются в Scene;
- но лишние не участвуют в текущем lighting calculation.

---

## 32. Point Shadow Mapping

Для Point Light используется depth cubemap.

Источник света требует шесть направлений:

```text
+X
-X
+Y
-Y
+Z
-Z
```

Каждая грань использует perspective projection с FOV 90°.

---

## 33. Shadow Pass

Pipeline:

```text
find shadow light
↓
create six Light VP matrices
↓
for each cubemap face:
    bind depth face
    clear depth
    render shadow-casting geometry
↓
restore QOpenGLWidget framebuffer
↓
restore viewport
↓
main color pass
```

---

## 34. QOpenGLWidget framebuffer

`QOpenGLWidget` не обязан использовать framebuffer 0.

После offscreen shadow pass необходимо восстанавливать:

```cpp
defaultFramebufferObject()
```

а не делать предположение:

```text
FBO == 0
```

---

## 35. HiDPI / Retina

Qt может сообщать Widget size в logical pixels, а OpenGL viewport использует physical framebuffer pixels.

Пример:

```text
width = 1200
height = 900
devicePixelRatioF = 2.0

physical framebuffer = 2400 × 1800
```

После shadow pass viewport должен восстанавливаться с учётом `devicePixelRatioF()`.

---

## 36. Camera

Управление камерой:

| Действие | Управление |
|---|---|
| Forward | `W` |
| Backward | `S` |
| Left | `A` |
| Right | `D` |
| Up | `E` |
| Down | `Q` |
| Rotate | `Alt + ЛКМ` + drag |
| Focus selected | `F` |
| Perspective | `1` или `P` |
| Orthographic | `2` или `O` |
| Zoom / dolly | wheel / trackpad |

Perspective scroll двигает Camera вдоль Forward.

Orthographic scroll меняет размер orthographic projection.

---

## 37. Input model

Keyboard events меняют состояния:

```text
move_forward
move_backward
move_left
move_right
move_up
move_down
```

`TickInput()` периодически применяет движение.

Такой подход обеспечивает непрерывный input, а не шаг только в момент key event.

---

## 38. Delta time и защита от teleport

Delta time должен быть ограничен сверху после:

- debugger pause;
- долгого stall;
- переключения окна;
- suspend/resume.

Это предотвращает большой jump Camera после возвращения приложения.

---

## 39. Focus handling

При потере focus input state должен сбрасываться.

Иначе возможна ошибка:

```text
W pressed
↓
window loses focus
↓
KeyRelease не приходит
↓
camera продолжает двигаться
```

---

## 40. Ray Picking

Object selection:

```text
mouse position
↓
viewport coordinates
↓
camera ray
↓
intersection tests
↓
nearest object
↓
selected_object
```

Gizmo interaction имеет приоритет над повторным object picking, когда пользователь попал по gizmo axis.

---

## 41. Gizmo

Editor поддерживает:

- Move;
- Rotate;
- Scale.

Gizmo является editor helper и не должен становиться обычным SceneObject.

Move/Rotate могут работать в World Space.

Scale gizmo может учитывать local object rotation.

---

## 42. BoundingBox

BoundingBox используется для:

- picking;
- model fitting;
- camera frame selected;
- spatial calculations.

Локальный BoundingBox imported mesh должен преобразовываться Model Matrix при world-space вычислениях.

---

## 43. Frame Selected

`F` фокусирует Camera на selected object.

Алгоритм должен учитывать:

- local bounding box;
- object transform;
- world center;
- approximate world size.

---

## 44. EditorWindow

`EditorWindow` — основной Qt orchestration layer.

Он отвечает за:

- menu bar;
- QAction;
- dock panels;
- central SceneViewport;
- status bar;
- Tests dialog;
- синхронизацию UI после create/load/clear/test.

В EditorWindow не должна переноситься математика Renderer.

---

## 45. Hierarchy

Hierarchy отображает SceneObjects.

Требования:

- refresh после изменения Scene;
- выбор объекта;
- синхронизация с SceneViewport;
- сброс selection после Clear/Load/Test, когда это необходимо.

---

## 46. Inspector

Inspector редактирует свойства выбранного SceneObject.

Ожидаемые секции:

- object name/type;
- Transform;
- Material;
- PointLight properties;
- другие доступные компоненты.

Inspector должен изменять реальные данные SceneObject.

---

## 47. Project Panel

Project Panel показывает project assets/imported resources.

Asset и SceneObject — разные понятия.

Файл может существовать в Project, даже если объект сейчас не создан в Scene.

---

## 48. OBJ Import

Поддерживаемый внешний формат:

```text
Wavefront OBJ
```

Face formats:

```text
v
v/vt
v//vn
v/vt/vn
```

Parser должен обрабатывать:

- positions;
- texture coordinates;
- normals;
- indices;
- polygon faces;
- triangulation;
- отрицательные индексы OBJ, если они поддержаны текущей реализацией.

---

## 49. Model import pipeline

```text
OBJ file
↓
ObjParser
↓
ImportedMeshData
↓
Mesh
↓
SceneObject
↓
Scene
```

GPU resource creation должна происходить при активном OpenGL Context.

---

## 50. Model Fit

Imported model может автоматически fit-иться в удобный размер.

Общий алгоритм:

```text
collect bounds
↓
find global min/max
↓
calculate center
↓
calculate maximum dimension
↓
compute uniform scale
↓
place near spawn point
```

---

## 51. Scene serialization

Формат сцены:

```text
*.scene
```

Система должна поддерживать:

- Save Scene;
- Open Scene;
- восстановление objects;
- восстановление transform;
- восстановление доступных component/material data в пределах текущего формата.

После Load UI должен быть синхронизирован с новой Scene.

---

## 52. OpenGL resource lifecycle

GPU resources должны уничтожаться при current OpenGL Context.

Правильный lifecycle:

```text
makeCurrent()
↓
destroy Mesh/Texture/Shader/Shadow resources
↓
doneCurrent()
```

---

## 53. Benchmark subsystem

Tests Dialog является встроенным performance harness.

Параметры:

```text
Test Scene:
1 000 Cubes
10 000 Cubes
25 000 Cubes
50 000 Cubes

Execution Mode:
Single Thread
Multi Thread
```

Показываются:

- Objects;
- Workers;
- Update;
- Render;
- Frame;
- FPS.

---

## 54. Benchmark lifecycle

```text
Idle
 ↓ Run
Warm-up
 ↓ 2 sec
Measurement
 ↓ 5 sec
Finished
 ↓ Stop
Restore user scene
```

---

## 55. Warm-up

Первые 2 секунды не входят в итоговую статистику.

Цель:

- убрать первые нестабильные кадры;
- снизить влияние cold state;
- стабилизировать caches/driver behavior.

---

## 56. Measurement window

Следующие 5 секунд используются для итоговых средних значений.

Накопители:

```text
update_sum_ms
render_sum_ms
frame_sum_ms
sample_count
elapsed_ms
```

Среднее:

```text
average = sum / sample_count
```

FPS:

```text
sample_count * 1000 / elapsed_ms
```

---

## 57. Benchmark results

| Objects | Mode | Workers | Update | Render | Frame | FPS |
|---:|---|---:|---:|---:|---:|---:|
| 1 000 | Single | 1 | 0.02 ms | 4.88 ms | 4.90 ms | 203.09 |
| 1 000 | Multi | 10 | 0.04 ms | 5.08 ms | 5.11 ms | 195.6 |
| 10 000 | Single | 1 | 0.12 ms | 33.62 ms | 33.74 ms | 29.6 |
| 10 000 | Multi | 10 | 0.06 ms | 33.65 ms | 33.71 ms | 29.7 |
| 25 000 | Single | 1 | 0.73 ms | 84.57 ms | 85.30 ms | 11.7 |
| 25 000 | Multi | 10 | 0.26 ms | 85.00 ms | 85.27 ms | 11.7 |
| 50 000 | Single | 1 | 1.20 ms | 171.97 ms | 173.17 ms | 5.8 |
| 50 000 | Multi | 10 | 0.58 ms | 172.40 ms | 172.99 ms | 5.8 |

---

## 58. Интерпретация benchmark

### 1 000 объектов

```text
Single = 0.02 ms
Multi  = 0.04 ms
```

Multi Thread хуже, потому что overhead ThreadPool больше полезной работы.

### 10 000 объектов

```text
0.12 ms → 0.06 ms
```

Примерно двукратное ускорение CPU Update.

### 25 000 объектов

```text
0.73 ms → 0.26 ms
```

Ускорение примерно `2.8x`.

### 50 000 объектов

```text
1.20 ms → 0.58 ms
```

Ускорение примерно `2.1x`.

---

## 59. Почему FPS почти не меняется

На 50 000 объектов:

```text
Update ≈ 0.6–1.2 ms
Render ≈ 172 ms
```

Поэтому:

```text
Frame ≈ Render
```

Multi Thread ускоряет CPU Update, но не Render.

Главный bottleneck больших stress scenes — rendering cost.

---

## 60. Ограничения Renderer

При большом числе SceneObjects возникает большое число отдельных draw calls.

Пример:

```text
50 000 cubes
→ potentially tens of thousands of draw calls
```

Текущая архитектура ещё не ориентирована на максимальную GPU batching efficiency.

---

## 61. Направления оптимизации Renderer

Следующие возможные этапы:

- instanced rendering;
- batching;
- frustum culling;
- material sorting;
- render command buffers;
- visibility system;
- reduced state changes.

---

## 62. ExecutionMode::Auto

Планируемый режим:

```cpp
enum class ExecutionMode {
    SingleThreaded,
    MultiThreaded,
    Auto
};
```

Идея:

```text
small workload
→ Single

large workload
→ Multi
```

Threshold не должен выбираться случайно.

Для точного crossover рекомендуется дополнительный benchmark:

```text
2.5k
5k
7.5k
10k
```

---

## 63. Критерии готовности Multi Thread

- ThreadPool запускает workers;
- worker count отображается;
- 1k/10k/25k/50k benchmark работает;
- отсутствует зависание;
- отсутствует очевидный deadlock;
- Transform обновляется корректно;
- Render запускается после WaitIdle;
- OpenGL остаётся на main thread;
- Stop Test восстанавливает пользовательскую Scene.

---

## 64. Критерии готовности Renderer текущего этапа

- Cube/Plane/Sphere отображаются;
- OBJ отображается;
- materials работают;
- textures работают;
- Point Light освещает geometry;
- shadow корректно блокируется препятствием;
- grid/axes отображаются;
- shadow pass восстанавливает правильный framebuffer;
- viewport корректен на Retina/HiDPI.

---

## 65. Критерии готовности Editor

- окно запускается;
- SceneViewport работает;
- Hierarchy работает;
- Inspector работает;
- Project panel работает;
- объект можно создать;
- объект можно выбрать;
- Transform можно изменить;
- Point Light можно создать;
- OBJ можно импортировать;
- Scene можно save/load;
- benchmark запускается;
- benchmark останавливается;
- обычная Scene восстанавливается.

---
