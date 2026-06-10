package org.example.domain.sensor

/**
 * Value Object — Corrente eléctrica em Ampere.
 * Lida pelo INA226 no subsistema EPS. Intervalo esperado: 0–3 A.
 */
@JvmInline
value class Current(val amperes: Double) {

    init {
        require(amperes >= 0.0) { "Corrente não pode ser negativa: $amperes A" }
        require(amperes <= MAX_AMPERES) { "Corrente demasiado alta: $amperes A (max $MAX_AMPERES A)" }
    }

    val milliamperes: Double get() = amperes * 1000.0

    fun powerWith(voltage: Voltage): Double = amperes * voltage.volts   // Watt

    fun isCritical(): Boolean = amperes > CRITICAL_HIGH

    override fun toString(): String = "%.3f A".format(amperes)

    companion object {
        const val MAX_AMPERES   = 5.0
        const val CRITICAL_HIGH = 2.5

        fun ofAmperes(value: Double)      = Current(value)
        fun ofMilliamperes(value: Double) = Current(value / 1000.0)
    }
}
