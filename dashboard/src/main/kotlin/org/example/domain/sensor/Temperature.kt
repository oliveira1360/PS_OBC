package org.example.domain.sensor

/**
 * Value Object — Temperatura em graus Celsius.
 * Garante que o valor está dentro dos limites físicos do sensor de bordo
 * (ADT7422: -40 °C a +150 °C).
 */
@JvmInline
value class Temperature(val celsius: Double) {

    init {
        require(celsius in RANGE) {
            "Temperatura inválida: $celsius °C. Intervalo permitido: $RANGE"
        }
    }

    val kelvin: Double get() = celsius + 273.15
    val fahrenheit: Double get() = celsius * 9.0 / 5.0 + 32.0

    fun isCritical(): Boolean = celsius > CRITICAL_HIGH || celsius < CRITICAL_LOW

    override fun toString(): String = "%.2f °C".format(celsius)

    companion object {
        private val RANGE = -50.0..150.0
        const val CRITICAL_HIGH = 85.0
        const val CRITICAL_LOW  = -30.0

        fun ofCelsius(value: Double)    = Temperature(value)
        fun ofKelvin(value: Double)     = Temperature(value - 273.15)
        fun ofFahrenheit(value: Double) = Temperature((value - 32.0) * 5.0 / 9.0)
    }
}
