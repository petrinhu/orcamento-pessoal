#pragma once

#include "models/PrecoAtivo.h"
#include <QString>

// Parser puro (sem Qt Network/SQL/QObject) do layout posicional B3 COTAHIST
// (registro tipo "01" -- cotação diária de ativo). Extraído de
// CotahistProvider (#199, Onda U, QA-01) seguindo o mesmo padrão de
// BankPDFParsers: função livre, testável isoladamente com fixture de linha
// real, sem precisar instanciar rede/DB.
//
// CotahistProvider.cpp/.h permanecem FORA do CMakeLists do app (Bolsa
// congelada); este parser é o único pedaço da cadeia COTAHIST que o
// CMakeLists de testes compila.
namespace cotahist {

// Registro tipo "01" (cotação) tem >= 245 chars no layout B3. Rejeita linhas
// curtas ou de tipo diferente (ex. "99" = trailer do arquivo).
bool isValidRecord(const QString &line);

// Extrai ticker/data/OHLCV+volume de uma linha COTAHIST tipo "01".
// PrecoAtivo::ticker vazio == linha inválida/não parseável.
PrecoAtivo parseLine(const QString &line);

} // namespace cotahist
