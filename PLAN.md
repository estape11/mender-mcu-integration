# Plan de upgrade: Zephyr 4.2.0 → 4.4.0

Documento de trabajo para el side project de actualización de la base de
Zephyr en `mender-mcu-integration` y módulos asociados.

## Objetivo

Migrar el manifest `west.yml` y el código de `mender-mcu-integration`
desde Zephyr **v4.2.0** a **v4.4.0** manteniendo la funcionalidad actual
demostrada en el branch `tmp/4.2.0`:

- Boot del ESP32-S3 DevKitC.
- Conexión WiFi (WPA2-PSK, 2.4 GHz).
- DHCPv4.
- Autenticación y comunicación con el servidor de Mender.

Beneficios esperados de subir versión:

- Posible resolución de bugs internos del HAL de Espressif que tocamos
  durante la sesión (typo en CMakeLists de `wpa_supplicant`, saturación
  de logs `DBG`, errores `memory allocation failed` del adaptador WiFi).
- Mejor soporte de WPA3 / PMF en routers en modo mixto.
- Mejoras generales del subsistema de red y supplicant.

## Estado actual (snapshot)

| Componente | Versión / commit |
|---|---|
| `zephyr` | `v4.2.0` (`413b789de`) |
| `modules/hal/espressif` | `f3453bdec` ("wifi: fix internal malloc wrappers reference") |
| `modules/mender-mcu` | `main` (`cf31fa22b`) |
| `mender-mcu-integration` | `main` (`d588ff280`) |
| Zephyr SDK | `0.17.0` |
| Target | `esp32s3_devkitc/esp32s3/procpu` |

### Tags disponibles tras `git fetch zephyrproject-rtos --tags`

- **v4.4.0** (stable, target del upgrade)
- v4.4.0-rc1 / rc2 / rc3
- v4.3.0 (stable, paso intermedio si hace falta)
- v4.3.0-rc1 / rc2 / rc3
- v4.2.2 (patch release sobre 4.2.0 — opción de bajo riesgo si 4.4 explota)
- v4.2.1

### Remotes configurados

| Remote | URL | Propósito |
|---|---|---|
| `origin` | `https://github.com/mendersoftware/mender-mcu-integration` | Upstream oficial (read-only para nosotros) |
| `estape11` | `git@github.com:estape11/mender-mcu-integration.git` | Fork personal, donde viven las ramas del proyecto |

### Branches relevantes

| Branch | Ubicación | Propósito |
|---|---|---|
| `main` | `origin` + `estape11` (eventual) | Mainline, arranque del upgrade |
| `tmp/4.2.0` | `estape11` (pushed) | Snapshot funcional del baseline 4.2.0 con los fixes de WiFi |
| `chore/zephyr-4.4-phase-1` | local (esta rama actual) | Fase 1: reconocimiento + .gitignore |

## Estrategia: una rama por fase

Cada fase se desarrolla en su propia rama (`chore/zephyr-4.4-phase-N-...`),
con su propio PR mental (no necesariamente abrimos PR a `origin/main`,
solo a `estape11/main` o lo dejamos en la fork). Esto permite:

- Aislar el blast radius de cada cambio.
- Rebobinar a la rama anterior si una fase rompe algo irrecuperable.
- Revisar el diff de cada fase de forma independiente.

## Fase 1 — Reconocimiento

**Branch:** `chore/zephyr-4.4-phase-1`

**Objetivo:** entender qué cambió entre 4.2.0 y 4.4.0 sin tocar el build.

### Tareas

1. ✅ Fetch de tags en el repo de Zephyr (`git fetch zephyrproject-rtos --tags`).
   Confirmado: v4.4.0 stable existe.
2. ✅ Añadir `.gitignore` para `.env`, `.DS_Store`, `build/`.
3. ⏳ Leer release notes de v4.3.0 y v4.4.0 en
   `zephyr/doc/releases/release-notes-4.3.rst` y `release-notes-4.4.rst`.
   Foco:
   - Networking (DHCPv4, WiFi L2, supplicant).
   - HAL Espressif (cambios de revisión que arrastra el manifest).
   - MCUboot (cambios de signing / partitions).
   - Removals / deprecations de APIs que use `netup.c` o `mender-mcu`.
   - Cambios en Kconfigs que ya usamos (`CONFIG_ESP32_WIFI_*`,
     `CONFIG_NET_*`, `CONFIG_LOG_*`).
4. ⏳ Revisar el `west.yml` de Zephyr v4.4.0 (en el commit del tag)
   para anotar qué revisión de `hal_espressif`, `mbedtls`, `mcuboot`
   trae — comparar con lo que tenemos hoy.
5. ⏳ Verificar compatibilidad de `mender-mcu/main` con Zephyr 4.4:
   - Mirar `mender-mcu/west.yml` o documentación.
   - Buscar en el repo de Mender PRs / issues mencionando "4.4" o
     "Zephyr 4.4".
6. ⏳ Confirmar versión mínima del **Zephyr SDK** requerida por v4.4 —
   `zephyr/SDK_VERSION` o el script `west_commands/sdk.py`. Si pide
   > 0.17.0, anotar para Fase 2.
7. ⏳ Hacer una lista concreta de **riesgos identificados** y, donde
   sea posible, de **fixes esperados** (bugs que tocamos durante la
   sesión y que el bump podría resolver).

### Entregable de Fase 1

Un commit en `chore/zephyr-4.4-phase-1` con:

- `.gitignore` (ya hecho).
- Actualización a este `PLAN.md` con la sección "Hallazgos de Fase 1"
  rellena (ver al final del documento).

**No** se toca `west.yml` en esta fase.

## Fase 2 — Bump del manifest

**Branch:** `chore/zephyr-4.4-phase-2-bump-manifest` (desde
`chore/zephyr-4.4-phase-1` o desde `main` con cherry-pick del `.gitignore`,
a decidir al iniciar la fase).

### Tareas

1. En `west.yml`, cambiar:
   ```yaml
   - name: zephyr
     revision: v4.2.0
   ```
   por:
   ```yaml
   - name: zephyr
     revision: v4.4.0
   ```
2. `west update` — re-resuelve todos los módulos (`hal_espressif`,
   `mbedtls`, `mcuboot`, etc.) a las revisiones que el manifest de
   Zephyr 4.4 declare.
3. Build limpio:
   ```sh
   west build -p always -b esp32s3_devkitc_procpu/esp32s3/procpu \
       mender-mcu-integration
   ```
4. Recoger **todos** los errores/warnings y clasificarlos:
   - Build errors (CMake / compilation / link).
   - Kconfig warnings (símbolos que ya no existen, defaults nuevos).
   - SDK mismatch (si exige > 0.17.0).
5. Commit del cambio de `west.yml` + cualquier ajuste mínimo de Kconfig
   necesario para que `west build` al menos llegue a la fase de link.

### Entregable de Fase 2

`west.yml` apuntando a `v4.4.0` + una lista priorizada de errores que
toca resolver en Fase 3.

## Fase 3 — Resolver breakage

**Branch:** `chore/zephyr-4.4-phase-3-fixes` (desde Fase 2).

### Tareas

Por cada categoría de error de Fase 2:

1. **APIs de net cambiadas**: revisar `src/utils/netup.c`. Los
   símbolos sospechosos son
   `net_mgmt_init_event_callback`, `net_mgmt_add_event_callback`,
   `net_dhcpv4_start`, `wifi_connect_req_params`, las constantes
   `NET_EVENT_IPV4_ADDR_ADD` y `NET_EVENT_WIFI_*`. Para cada uno,
   verificar firma en `zephyr/include/zephyr/net/*` de la 4.4.
2. **HAL Espressif**: si hay cambios en `esp_wifi_drv.c` o en los
   `wifi_config_t` / `wifi_connect_req_params`, adaptar.
3. **MCUboot**: regenerar la firma o ajustar configs si la 4.4 cambió
   el formato de imágenes.
4. **mender-mcu**: si la lib usa APIs cambiadas, decidir si pinear
   `mender-mcu` a un commit conocido-bueno o subirlo a su `main` actual.
5. Verificar que los **bugs conocidos** que vimos en 4.2.0 estén
   arreglados:
   - Typo `wpa_supplicantesp_supplicant` en
     `modules/hal/espressif/zephyr/esp32s3/CMakeLists.txt:578` ⇒
     hacer grep en el HAL nuevo.
   - Saturación del logging con `DBG` ⇒ probar el build con `DBG`
     activado y ver si sigue colgando.
   - `esp32_wifi_adapter: memory allocation failed` ⇒ correr Mender
     durante varios ciclos y vigilar.

### Entregable de Fase 3

Build verde, sin errores ni warnings nuevos relevantes.

## Fase 4 — Verificación funcional

**Branch:** misma de Fase 3.

### Tareas

1. Flashear: `west flash`.
2. Boot del ESP32-S3 y captura del log desde
   `Booting Zephyr OS build v4.4.0`.
3. Checklist:
   - [ ] Banner muestra `v4.4.0` (no 4.2.0).
   - [ ] Asocia con `Fam_AB_iot` (Nexxt) a la primera o tras retry.
   - [ ] DHCPv4 entrega IP.
   - [ ] Mender se autentica con el server.
   - [ ] `Checking for deployment...` → `No deployment available` (sin
         409 ni errores TLS).
   - [ ] Tras ~5 minutos corriendo, **no** aparecen `memory allocation
         failed` (test de estabilidad).
4. Pruebas de regresión:
   - Probar con `Fam_AB` (router de casa, WPA2/WPA3 mixto). Si la 4.4
     arregla el flake de AUTH_FAIL (202), ganancia inesperada y muy
     bienvenida.
   - Probar al menos un deployment real (subir un artefacto al server
     y forzar update).

### Entregable de Fase 4

Captura del log de boot funcional en v4.4.0 + sumario de comparación con
v4.2.0 (en `PLAN.md`, sección "Resultado de verificación").

## Fase 5 — Decisión de merge

**Branch:** decisión.

### Opciones

1. **Merge a `estape11/main`**: si la 4.4 funciona y trae mejoras,
   convertir el upgrade en el nuevo `main` del fork.
2. **Mantener en rama y abrir PR upstream**: si el upgrade es lo bastante
   limpio, considerar abrir PR a `mendersoftware/mender-mcu-integration`.
3. **Aparcar la rama**: si la 4.4 no aporta o introduce regresiones,
   dejar la rama como referencia y volver a `tmp/4.2.0` como baseline
   estable.

La decisión se toma con los datos de Fase 4 en mano, no antes.

## Riesgos transversales

| Riesgo | Probabilidad | Mitigación |
|---|---|---|
| HAL Espressif rompe APIs internas | Alta | Aislar en Fase 3, cherry-pick fixes del HAL si los hay |
| mender-mcu/main no soporta 4.4 todavía | Media | Pinear a un commit anterior conocido-bueno, o esperar a que Mender bumpee |
| SDK 0.17.0 insuficiente para 4.4 | Media | Instalar SDK más reciente (no es destructivo, conviven) |
| Cambios de Kconfig defaults | Alta | Comparar `zephyr/.config` antes/después; documentar overrides |
| Regresión silenciosa en supplicant (el flake que vimos puede empeorar) | Media | Test de estabilidad de Fase 4 con >5 min de uptime |
| Lock-in del fork si abrimos PR upstream | Baja | Decisión separada en Fase 5 |

## Cosas que NO entran en este proyecto

Para mantener el scope acotado:

- Migración a otras boards (`nrf52840dk`, `native_sim`, etc.). Solo
  `esp32s3_devkitc_procpu`.
- Cambios funcionales en la app (`main.c`, callbacks de Mender). Solo
  cambios estrictamente necesarios para que compile y corra en 4.4.
- Refactor de `netup.c` para timeouts/retries a nivel de aplicación
  (lo dejamos como futuro work).
- Cambiar el módulo de actualización (`zephyr-image` sigue igual).

## Convenciones operativas

- Conventional commits: `chore:`, `feat:`, `fix:`, etc., siguiendo el
  estilo del repo upstream.
- Cada rama de fase se pushea a `estape11/<branch-name>` cuando llega a
  un estado coherente (build limpio o checkpoint útil).
- `main` (local) **no** se modifica hasta Fase 5.
- `tmp/4.2.0` se mantiene intacto como fallback.

---

## Hallazgos de Fase 1

_(Pendiente — se irá rellenando a medida que se ejecuten las tareas 3–7
de Fase 1.)_

### Release notes v4.3 — diff vs 4.2

_TBD_

### Release notes v4.4 — diff vs 4.3

_TBD_

### Revisiones del manifest de Zephyr v4.4.0

_TBD_

### Compatibilidad mender-mcu/main con Zephyr 4.4

_TBD_

### SDK mínimo requerido

_TBD_

### Riesgos identificados específicos

_TBD_

### Bugs de 4.2.0 que podrían arreglarse

_TBD_
