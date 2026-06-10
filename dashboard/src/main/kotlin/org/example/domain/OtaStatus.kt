package org.example.domain

data class OtaStatus(
    val inProgress: Boolean = false,
    val progress: Int = 0,
    val totalRecords: Int = 0,
    val sentRecords: Int = 0,
    val success: Boolean? = null,
    val message: String = ""
)
