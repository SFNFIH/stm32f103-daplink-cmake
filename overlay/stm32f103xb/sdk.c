/**
 * @file    sdk.c
 * @brief   STM32F103 clocks + early USB disconnect
 *
 * Pick PLL once before USB attaches. Do not retune in USBD_Connect — switching
 * the 48 MHz USB clock while the host enumerates causes error -71 / truncated
 * descriptors (probabilistic attach).
 *
 * HSE frequency is measured vs HSI (RTC = HSE/128) and mapped to 8/12/16 MHz.
 * If HSE is absent, HSI/2 × 12 = 48 MHz USB.
 */

#include "stm32f1xx.h"
#include "IO_Config.h"
#include "DAP_config.h"
#include "gpio.h"
#include "daplink.h"
#include "util.h"
#include "cortex_m.h"

TIM_HandleTypeDef timer;
uint32_t time_count;

static uint32_t tim2_clk_div(uint32_t apb1clkdiv);

static void usb_hold_disconnected(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_AFIOEN;
    __HAL_AFIO_REMAP_SWJ_NOJTAG();
    /* PA15 output 50 MHz push-pull */
    GPIOA->CRH = (GPIOA->CRH & ~(0xFul << 28)) | (0x3ul << 28);
    USB_CONNECT_OFF();
}

static HAL_StatusTypeDef sysclk_from_pll(uint32_t flash_latency)
{
    RCC_ClkInitTypeDef clk = {0};

    clk.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK |
                     RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV2;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    return HAL_RCC_ClockConfig(&clk, flash_latency);
}

static void usbclk_48mhz(int pll_is_72mhz)
{
    if (pll_is_72mhz) {
        __HAL_RCC_USB_CONFIG(RCC_USBCLKSOURCE_PLL_DIV1_5);
    } else {
        __HAL_RCC_USB_CONFIG(RCC_USBCLKSOURCE_PLL);
    }
}

static int switch_to_hsi(void)
{
    RCC_ClkInitTypeDef clk = {0};

    clk.ClockType = RCC_CLOCKTYPE_SYSCLK;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    return HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_1) == HAL_OK;
}

static int hse_on(void)
{
    RCC_OscInitTypeDef osc = {0};

    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState = RCC_HSE_ON;
    osc.PLL.PLLState = RCC_PLL_NONE;
    if (HAL_RCC_OscConfig(&osc) == HAL_OK) {
        return 1;
    }
    osc.HSEState = RCC_HSE_BYPASS;
    return HAL_RCC_OscConfig(&osc) == HAL_OK;
}

/* SYSCLK must be HSI. Returns 0 on timeout. */
static uint32_t measure_hse_hz(void)
{
    const uint32_t rtc_ticks = 16u;
    const uint32_t hse_cycles = rtc_ticks * 128u;
    uint32_t t0;
    uint32_t t1;
    uint32_t dwt;
    uint32_t guard;

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_RCC_BKP_CLK_ENABLE();
    SET_BIT(PWR->CR, PWR_CR_DBP);

    SET_BIT(RCC->BDCR, RCC_BDCR_BDRST);
    CLEAR_BIT(RCC->BDCR, RCC_BDCR_BDRST);

    MODIFY_REG(RCC->BDCR, RCC_BDCR_RTCSEL, RCC_BDCR_RTCSEL_HSE);
    SET_BIT(RCC->BDCR, RCC_BDCR_RTCEN);

    CLEAR_BIT(RTC->CRL, RTC_CRL_RSF);
    guard = 200000u;
    while ((RTC->CRL & RTC_CRL_RSF) == 0u) {
        if (--guard == 0u) {
            return 0;
        }
    }

    guard = 200000u;
    while ((RTC->CRL & RTC_CRL_RTOFF) == 0u) {
        if (--guard == 0u) {
            return 0;
        }
    }
    RTC->CRL |= RTC_CRL_CNF;
    RTC->PRLH = 0;
    RTC->PRLL = 0;
    RTC->CRL &= (uint16_t)~RTC_CRL_CNF;
    guard = 200000u;
    while ((RTC->CRL & RTC_CRL_RTOFF) == 0u) {
        if (--guard == 0u) {
            return 0;
        }
    }

    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    t0 = RTC->CNTL & 0xFFFFu;
    guard = 2000000u;
    while ((RTC->CNTL & 0xFFFFu) == t0) {
        if (--guard == 0u) {
            return 0;
        }
    }

    t0 = RTC->CNTL & 0xFFFFu;
    DWT->CYCCNT = 0;
    guard = 2000000u;
    do {
        t1 = RTC->CNTL & 0xFFFFu;
        if (--guard == 0u) {
            return 0;
        }
    } while ((uint16_t)(t1 - t0) < rtc_ticks);

    dwt = DWT->CYCCNT;
    CLEAR_BIT(RCC->BDCR, RCC_BDCR_RTCEN);
    if (dwt < 200u) {
        return 0;
    }
    return (uint32_t)((8000000ull * (uint64_t)hse_cycles) / (uint64_t)dwt);
}

static int pll_hse(uint32_t prediv, uint32_t mul)
{
    RCC_OscInitTypeDef osc = {0};

    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState = (RCC->CR & RCC_CR_HSEBYP) ? RCC_HSE_BYPASS : RCC_HSE_ON;
    osc.HSEPredivValue = prediv;
    osc.PLL.PLLState = RCC_PLL_ON;
    osc.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    osc.PLL.PLLMUL = mul;
    if (HAL_RCC_OscConfig(&osc) != HAL_OK) {
        return 0;
    }
    if (sysclk_from_pll(FLASH_LATENCY_2) != HAL_OK) {
        return 0;
    }
    usbclk_48mhz(1);
    (void)HAL_InitTick(0);
    return 1;
}

static int pll_hsi_48mhz(void)
{
    RCC_OscInitTypeDef osc = {0};

    osc.OscillatorType = RCC_OSCILLATORTYPE_HSI | RCC_OSCILLATORTYPE_HSE;
    osc.HSEState = RCC_HSE_OFF;
    osc.HSIState = RCC_HSI_ON;
    osc.PLL.PLLState = RCC_PLL_OFF;
    (void)HAL_RCC_OscConfig(&osc);

    osc = (RCC_OscInitTypeDef){0};
    osc.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    osc.HSIState = RCC_HSI_ON;
    osc.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
    osc.PLL.PLLState = RCC_PLL_ON;
    osc.PLL.PLLMUL = RCC_PLL_MUL12;
    if (HAL_RCC_OscConfig(&osc) != HAL_OK ||
        sysclk_from_pll(FLASH_LATENCY_1) != HAL_OK) {
        return 0;
    }
    usbclk_48mhz(0);
    (void)HAL_InitTick(0);
    return 1;
}

void sdk_init(void)
{
    uint32_t hse_hz = 0;
    int ok = 0;

    usb_hold_disconnected();
    SystemCoreClockUpdate();
    HAL_Init();

    if (!switch_to_hsi()) {
        util_assert(0);
        return;
    }

    if (hse_on()) {
        hse_hz = measure_hse_hz();
    }

    if ((hse_hz > 14000000u) && (hse_hz < 18000000u)) {
        ok = pll_hse(RCC_HSE_PREDIV_DIV2, RCC_PLL_MUL9); /* 16 MHz */
    } else if ((hse_hz > 10000000u) && (hse_hz < 14000000u)) {
        ok = pll_hse(RCC_HSE_PREDIV_DIV1, RCC_PLL_MUL6); /* 12 MHz */
    } else if ((hse_hz > 6000000u) && (hse_hz < 10000000u)) {
        ok = pll_hse(RCC_HSE_PREDIV_DIV1, RCC_PLL_MUL9); /* 8 MHz */
    } else if (hse_hz == 0u && (RCC->CR & RCC_CR_HSERDY)) {
        /* Measure failed but HSE runs: DAPLink default 8 MHz × 9 */
        ok = pll_hse(RCC_HSE_PREDIV_DIV1, RCC_PLL_MUL9);
    }

    if (!ok) {
        ok = pll_hsi_48mhz();
    }
    if (!ok) {
        util_assert(0);
    }
}

HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
    HAL_StatusTypeDef ret;
    RCC_ClkInitTypeDef clk_init;
    uint32_t unused;
    uint32_t prescaler;
    uint32_t source_clock;

    (void)TickPriority;

    HAL_RCC_GetClockConfig(&clk_init, &unused);

    source_clock = SystemCoreClock / tim2_clk_div(clk_init.APB1CLKDivider);
    prescaler = (uint32_t)(source_clock / 4000) - 1;

    timer.Instance = TIM2;
    timer.Init.Period            = 0xFFFF;
    timer.Init.Prescaler         = prescaler;
    timer.Init.ClockDivision     = 0;
    timer.Init.CounterMode       = TIM_COUNTERMODE_UP;
    timer.Init.RepetitionCounter = 0;

    __HAL_RCC_TIM2_CLK_ENABLE();

    ret = HAL_TIM_Base_DeInit(&timer);
    if (ret != HAL_OK) {
        return ret;
    }

    time_count = 0;
    ret = HAL_TIM_Base_Init(&timer);
    if (ret != HAL_OK) {
        return ret;
    }

    ret = HAL_TIM_Base_Start(&timer);
    if (ret != HAL_OK) {
        return ret;
    }

    return HAL_OK;
}

void HAL_IncTick(void)
{
}

uint32_t HAL_GetTick(void)
{
    cortex_int_state_t state;
    state = cortex_int_get_and_disable();
    const uint32_t ticks = __HAL_TIM_GET_COUNTER(&timer) / 4;
    time_count += (ticks - time_count) & 0x3FFF;
    cortex_int_restore(state);
    return time_count;
}

void HAL_SuspendTick(void)
{
    HAL_TIM_Base_Start(&timer);
}

void HAL_ResumeTick(void)
{
    HAL_TIM_Base_Stop(&timer);
}

static uint32_t tim2_clk_div(uint32_t apb1clkdiv)
{
    switch (apb1clkdiv) {
        case RCC_CFGR_PPRE1_DIV2:
            return 1;
        case RCC_CFGR_PPRE1_DIV4:
            return 2;
        case RCC_CFGR_PPRE1_DIV8:
            return 4;
        case RCC_CFGR_PPRE1_DIV16:
            return 8;
        default:
            return 1;
    }
}
