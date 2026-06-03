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

### Versiones que arrastra `zephyr/west.yml` en `v4.4.0`

| Módulo | Revisión 4.4.0 | Delta vs estado actual |
|---|---|---|
| `hal_espressif` | `b7953b80` | ~112 commits adelante respecto a `f3453bdec` |
| `mbedtls` | `a3e190fe` | Salto mayor — la 4.4 trae Mbed TLS **4.1.0** (cambio de versión mayor) |
| `mcuboot` | `ee39e2d6` | Cambio de modo de swap por defecto + nuevos defaults |
| `cmsis_6` | `30a859f4` | Probablemente irrelevante para nosotros (ARM Cortex) |
| `hal_nordic` / `hal_nxp` | varios | Irrelevantes para target ESP32-S3 |

### Bugs de 4.2.0 que la 4.4.0 sí arregla

| Bug visto en sesión | Commit upstream | Notas |
|---|---|---|
| Typo `wpa_supplicantesp_supplicant` en `hal_espressif/zephyr/esp32s3/CMakeLists.txt:578` que rompía el build al activar `CONFIG_ESP32_WIFI_ENABLE_WPA3_SAE` | `2a757f62bf` ("cmake: fix the path of fastpsk.c in CRYPTO_SRCS") | Arreglado en los 5 SoCs (esp32c2/c3/c6/s2/s3) |
| WiFi debug logging que causaba saturación / aparente cuelgue tras DHCP DISCOVER | `c870daf7ae` ("zephyr: fix esp_log_writev for Wi-Fi debug logging") | Sospechoso de ser relevante — verificar en Fase 4 reactivando `WIFI_LOG_LEVEL_DBG` |
| `esp32_wifi_adapter: memory allocation failed` (heap insuficiente) | varios, sin commit único — la sumatoria de fixes al adapter podría aliviarlo | Probar con heap a SYSTEM y observar |

### Breaking changes que nos afectan (4.2 → 4.4)

#### 1. SDK mínimo: 1.0.0 (vs 0.17.0 que tenemos)

> The minimum required Zephyr SDK version is now 1.0.0.
> (`migration-guide-4.4.rst:26`)

Habrá que instalar **Zephyr SDK 1.0.0** antes de la Fase 2. Conviven
varias versiones del SDK; no es destructivo.

#### 2. Mbed TLS bump masivo a 4.1.0 + TF-PSA-Crypto separado

`migration-guide-4.4.rst:1448` — Mbed TLS upgraded a 4.1.0. El módulo
`mbedtls` ahora solo contiene TLS y X.509; la parte de crypto se movió a
un módulo separado **TF-PSA-Crypto 1.1.0**. Se requiere migración
explícita siguiendo el documento upstream:
https://github.com/Mbed-TLS/TF-PSA-Crypto/blob/development/docs/1.0-migration-guide.md

**Impacto directo en `mender-mcu-integration/prj.conf`** (líneas 32–53):

Kconfigs que **ya no existen** en 4.4 y están en nuestro `prj.conf`:

- `CONFIG_MBEDTLS_ECDH_C`
- `CONFIG_MBEDTLS_ECDSA_C`
- `CONFIG_MBEDTLS_ECP_C`
- `CONFIG_MBEDTLS_ECP_DP_SECP256R1_ENABLED`
- `CONFIG_MBEDTLS_ECP_DP_SECP384R1_ENABLED`
- `CONFIG_MBEDTLS_CIPHER_CCM_ENABLED`
- `CONFIG_MBEDTLS_CIPHER_GCM_ENABLED`
- `CONFIG_MBEDTLS_SHA384`
- `CONFIG_MBEDTLS_GENPRIME_ENABLED`

Kconfigs **renombrados** (4.3 / 4.4):

- `CONFIG_MBEDTLS_PEM_CERTIFICATE_FORMAT` → `CONFIG_MBEDTLS_PEM_PARSE_C` + `CONFIG_MBEDTLS_PEM_WRITE_C` + `CONFIG_MBEDTLS_BASE64_C` (4.4)
- `CONFIG_MBEDTLS_SERVER_NAME_INDICATION` → `CONFIG_MBEDTLS_SSL_SERVER_NAME_INDICATION` (4.4)
- `CONFIG_MBEDTLS_ENTROPY_POLL_ZEPHYR` → `CONFIG_MBEDTLS_PSA_DRIVER_GET_ENTROPY` (4.4)

Equivalentes PSA esperados (a verificar en Fase 2):

- `CONFIG_MBEDTLS_ECDH_C` ≈ `CONFIG_PSA_WANT_ALG_ECDH`
- `CONFIG_MBEDTLS_ECDSA_C` ≈ `CONFIG_PSA_WANT_ALG_ECDSA`
- `CONFIG_MBEDTLS_ECP_DP_SECP256R1_ENABLED` ≈ `CONFIG_PSA_WANT_ECC_SECP_R1_256`
- `CONFIG_MBEDTLS_ECP_DP_SECP384R1_ENABLED` ≈ `CONFIG_PSA_WANT_ECC_SECP_R1_384`
- `CONFIG_MBEDTLS_CIPHER_CCM_ENABLED` ≈ `CONFIG_PSA_WANT_ALG_CCM`
- `CONFIG_MBEDTLS_CIPHER_GCM_ENABLED` ≈ `CONFIG_PSA_WANT_ALG_GCM`
- `CONFIG_MBEDTLS_SHA384` ≈ `CONFIG_PSA_WANT_ALG_SHA_384`

#### 3. `NET_SOCKETS_SOCKOPT_TLS` deja de auto-seleccionar crypto

> Automatic selection of crypto Kconfigs has been removed from
> `NET_SOCKETS_SOCKOPT_TLS` as they strongly depend on the final
> application's needs.

Hay que **declarar explícitamente** los ciphersuites y la versión de TLS
deseados con `CONFIG_MBEDTLS_CIPHERSUITE_TLS_*` y
`CONFIG_MBEDTLS_SSL_PROTO_TLS1_2/1_3`.

#### 4. Networking APIs namespaced (4.4)

`migration-guide-4.4.rst:1220` — símbolos en `net_ip.h` y `socket.h`
ahora llevan prefijos `net_`, `NET_`, `ZSOCK_`. Existe `net_compat.h`
como shim para mantener compatibilidad — debería minimizar el impacto
en código existente, pero hay que vigilar warnings de redefinición.

#### 5. WiFi supplicant: PSA crypto por defecto

`release-notes-4.4.rst:642-643` — el supplicant usa PSA crypto por
defecto. Para preservar el comportamiento previo se puede deshabilitar
con `CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_MBEDTLS_PSA=n`, pero lo
recomendable es ir hacia PSA.

#### 6. MCUboot: modo de swap por defecto cambió en 4.3

`migration-guide-4.3.rst:486-492` — el default pasó de "swap using move"
a "swap using offset", lo que afecta el layout flash. En el repo de
`mender-mcu` ya hay un commit `eb5c6c0` ("chore: select
`MCUBOOT_MODE_SWAP_WITHOUT_SCRATCH` for the zephyr image UM") que sugiere
que esto **ya está manejado** en el mender-mcu/main vigente. Verificar.

#### 7. Mbed TLS Kconfigs renombrados en 4.3 (varios)

- `CONFIG_MBEDTLS_TLS_VERSION_1_2` → `CONFIG_MBEDTLS_SSL_PROTO_TLS1_2`
- `CONFIG_MBEDTLS_TLS_VERSION_1_3` → `CONFIG_MBEDTLS_SSL_PROTO_TLS1_3`
- `CONFIG_MBEDTLS_MD` → `CONFIG_MBEDTLS_MD_C`
- `CONFIG_MBEDTLS_DTLS` → `CONFIG_MBEDTLS_SSL_PROTO_DTLS`

(En nuestro `prj.conf` actual no aparecen, así que probablemente no nos
toca — pero conviene chequearlo en `mender-mcu/src/`.)

#### 8. C17 como estándar mínimo

`migration-guide-4.4.rst:33` — el toolchain del SDK 1.0.0 ya cumple, así
que no debería pedir cambios. Solo relevante si alguna parte del código
declara `-std=c11` o similar manualmente.

#### 9. `wifi_channel_info` gained a `band` field

`migration-guide-4.4.rst:1213` — cambio de tamaño de struct, requiere
recompilación. Nosotros no lo usamos directamente desde la app, pero el
driver sí — el recompile completo resuelve esto.

### Compatibilidad de `mender-mcu/main` con Zephyr 4.4

**No declarada explícitamente.** El último commit "feat: support Zephyr
X.Y.Z" en `mender-mcu` es `2d8e6341` ("feat: support Zephyr 4.2.0",
2025-08-26). No hay commits posteriores que mencionen 4.3 o 4.4.

Riesgo: el código de `mender-mcu` puede usar APIs que cambiaron entre
4.2 y 4.4 (especialmente Mbed TLS / PSA). Estrategia probable:

- Intentar el build con `mender-mcu` en `main` actual.
- Si falla por APIs cambiadas, abrir issue / PR upstream y, mientras
  tanto, mantener un fork temporal de `mender-mcu` con los fixes.

### Bugs en `hal_espressif` arreglados upstream (lista parcial)

Tras `git log f3453bdec..b7953b80` en `modules/hal/espressif`, commits
relevantes a tener en cuenta:

- `2a757f62bf` — fix path `fastpsk.c` ✅ (arregla nuestro typo)
- `c870daf7ae` — fix `esp_log_writev` para Wi-Fi debug logging ✅
- `b5ba34f7db` — fix PSA crypto support para wpa_supplicant
- `3c1e0ff4cd` — fix Wi-Fi/BT coexistence support
- `b7953b8019` — guard wpa_supplicant includes behind Wi-Fi
- 107 commits adicionales (build system, syncs, fixes menores)

### Resumen de riesgos específicos detectados

| Riesgo | Severidad | Mitigación |
|---|---|---|
| `prj.conf` lleno de Kconfigs `MBEDTLS_*` que ya no existen | Alta | Reescritura sistemática a equivalentes PSA en Fase 3 |
| `mender-mcu` sin soporte oficial declarado para 4.4 | Alta | Tener listo un fork de `mender-mcu` con fixes locales si hace falta |
| TLS deja de auto-seleccionar crypto, podría romper handshake con Hosted Mender | Media | Declarar explícitamente TLS 1.2/1.3 + ciphersuites en `prj.conf` |
| SDK 0.17.0 insuficiente — build fallará pronto si no se actualiza | Alta | Instalar Zephyr SDK 1.0.0 antes de Fase 2 |
| Mbed TLS 4.1.0 cambia el comportamiento de PEM parse / SNI | Media | Verificar que Mender sigue parseando los certs correctamente |
| Modo de MCUboot cambiado en 4.3 (swap mode default) | Baja | `mender-mcu` ya seleccionaba `SWAP_WITHOUT_SCRATCH` — probable no impacto |

### Acciones de salida de la Fase 1

1. Commit este `PLAN.md` actualizado (ya en branch `chore/zephyr-4.4-phase-1`).
2. **Antes de Fase 2:** instalar Zephyr SDK 1.0.0.
3. Crear branch `chore/zephyr-4.4-phase-2-bump-manifest` desde
   `chore/zephyr-4.4-phase-1` (para llevar el `.gitignore` y este plan).
4. Decidir si pineamos `mender-mcu` a un commit concreto o lo dejamos en
   `main` para Fase 2 (recomendación: dejar en `main` y ver qué pasa;
   pinear solo si rompe de forma irreproducible).
