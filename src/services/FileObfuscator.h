#pragma once

#include <QJsonObject>
#include <QString>

class QSqlDatabase;

// FileObfuscator (#91) - gera export anonimizado do banco para bug reports.
//
// Substitui campos identificaveis (nomes de pessoas, descricoes livres,
// categorias custom, etc.) por dummies determinísticos preservando estrutura
// e relacoes referenciais. Valores monetarios, datas e IDs sao mantidos
// (preservam a forma do bug numerico/temporal).
//
// Mapeamento ficticio:
//   - Fontes de renda      -> "Fonte 1", "Fonte 2", ...
//   - Pagadores            -> "Pessoa 1", "Pessoa 2", ...
//   - Categorias           -> "Categoria 1", "Categoria 2", ...
//   - Cartoes              -> "Cartao 1", "Cartao 2", ...
//   - Descricoes livres    -> "d_1", "d_2", ... (sequencial irreversível)
//
// ATENCAO (escopo de seguranca): este export NAO mascara valores monetarios,
// datas nem IDs (por design — preservam a forma do bug). Portanto NAO e
// anonimizacao forte: padroes de gasto/data ainda sao observaveis. Use apenas
// para bug reports onde o texto livre e o que precisa ser ocultado. Sem
// consumidor na UI no momento.
//
// Padrao de grisbi/src/file_obfuscate.c.
class FileObfuscator
{
public:
    // Le todas as tabelas financeiras do db aberto e retorna QJsonObject
    // com schema_version + dados anonimizados por tabela.
    // Throws QSqlError se db nao estiver aberto ou query falhar.
    [[nodiscard]] static QJsonObject obfuscate(QSqlDatabase &db);

    // Wrap conveniente: serializa o output de obfuscate() para arquivo.
    // Retorna true em sucesso. Caller decide path via QFileDialog.
    static bool saveToFile(const QString &filePath, QSqlDatabase &db,
                           QString *errorOut = nullptr);
};
