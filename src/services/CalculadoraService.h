#pragma once
#include <QString>
#include <QtTypes>

// Utility class for the calculator: display formatting and value conversion.
// All methods are stateless and safe to call from any thread.
class CalculadoraService {
public:
    // Format a double for display: up to 10 decimal places, no trailing zeros.
    // Uses ',' as decimal separator (pt-BR).
    static QString formatarDisplay(double valor, int decimais = -1);

    // Parse a display string back to double.
    // Handles: "1.234,56" (thousands + decimal), "1234,56", "1234.56".
    static double parsearDisplay(const QString &texto);

    // Convert display value to centavos (rounds to 2 decimal places).
    static qint64 toCentavos(double valor);

    // Convert centavos to a double suitable for display.
    static double fromCentavos(qint64 centavos);
};
