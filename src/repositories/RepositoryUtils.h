#pragma once

#include <QString>

// #146/#146-bis (FIN-01): colunas TEXT NOT NULL (com ou sem DEFAULT '') não
// aceitam NULL explícito — bindValue de um QString() nulo insere NULL, e o
// SQLite rejeita com "NOT NULL constraint failed" (o DEFAULT só vale quando a
// coluna é OMITIDA do INSERT, nunca quando ela recebe NULL explicitamente).
//
// QString null (isNull()==true) surge tipicamente de:
//   - sentinela de campo opcional em diálogo (QDateEdit + setSpecialValueText,
//     ex.: MetaDialog::meta()/DividaDialog::divida() → "sem prazo/vencimento");
//   - QString::clear() em branch condicional (ex.: RecorrenciaDialog,
//     InvestimentoDialog zerando um campo não aplicável ao tipo escolhido);
//   - struct de domínio default-construído (QString membro sem inicializador
//     é nulo, não apenas vazio).
//
// naoNulo() normaliza null -> "" (string vazia não-nula) para que o insert()/
// update() NUNCA falhe silenciosamente por causa disso — o widget que descarta
// o bool de retorno (bug irmão) deixava de perceber a perda de dado.
inline QString naoNulo(const QString &s)
{
    return s.isNull() ? QString::fromLatin1("") : s;
}
