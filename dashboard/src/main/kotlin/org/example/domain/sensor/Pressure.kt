package org.example.domain.sensor

/**
 * Value Object — Pressão atmosférica em hPa (hectopascal).
 * Sensor BMP390: intervalo 300–1250 hPa com resolução de 0.01 hPa.
 * Em órbita, valores próximos de 0 são normais (vácuo).
 */
@JvmInline
value class Pressure(val hPa: Double) {

    init {
        require(hPa >= 0.0) { "Pressão não pode ser negativa: $hPa hPa" }
    }

    val pascal: Double      get() = hPa * 100.0
    val bar: Double         get() = hPa / 1000.0
    val atm: Double         get() = hPa / 1013.25

    fun isVacuum(): Boolean = hPa < 1.0

    override fun toString(): String = "%.2f hPa".format(hPa)

    companion object {
        val STANDARD_ATMOSPHERE = Pressure(1013.25)
        val NEAR_VACUUM         = Pressure(0.001)

        fun ofHPa(value: Double)    = Pressure(value)
        fun ofPascal(value: Double) = Pressure(value / 100.0)
        fun ofBar(value: Double)    = Pressure(value * 1000.0)
    }
}
