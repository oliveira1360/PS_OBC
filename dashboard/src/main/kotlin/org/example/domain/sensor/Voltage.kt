package org.example.domain.sensor

/**
 * Value Object — Tensão do subsistema EPS em Volt.
 * LiPo 1S: 3.0 V (vazia) a 4.2 V (carregada).
 * O sistema aceita até 8 V (reguladores/solar).
 */
@JvmInline
value class Voltage(val volts: Double) {

    init {
        require(volts >= 0.0) { "Tensão não pode ser negativa: $volts V" }
        require(volts <= MAX_VOLTS) { "Tensão demasiado alta: $volts V (max $MAX_VOLTS V)" }
    }

    val millivolts: Double get() = volts * 1000.0

    /** Nível de carga estimado para uma célula LiPo 1S (3.0–4.2 V). */
    fun batteryPercent(): Int {
        val pct = ((volts - 3.0) / (4.2 - 3.0) * 100.0).coerceIn(0.0, 100.0)
        return pct.toInt()
    }

    fun isCritical(): Boolean = volts < CRITICAL_LOW
    fun isCharging(): Boolean = volts > CHARGED_THRESHOLD

    override fun toString(): String = "%.3f V".format(volts)

    companion object {
        const val MAX_VOLTS         = 8.0
        const val CRITICAL_LOW      = 3.1
        const val CHARGED_THRESHOLD = 4.15

        fun ofVolts(value: Double)      = Voltage(value)
        fun ofMillivolts(value: Double) = Voltage(value / 1000.0)
    }
}
