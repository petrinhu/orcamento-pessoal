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
//   - Descricoes livres    -> hash MD5 truncado (10 chars) prefixado por "d_"
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
