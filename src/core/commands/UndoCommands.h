#pragma once

#include <QUndoCommand>
#include <functional>

// Generic lambda-based undo command.
//
// Usage pattern — perform the operation BEFORE pushing, then push:
//
//   auto item = std::make_shared<T>(value);
//   repo.insert(*item);               // item->id is now set
//
//   if (m_undoStack)
//       m_undoStack->push(new LambdaCommand(
//           tr("Adicionar X"),
//           [item]() { item->id = 0; Repo().insert(*item); },   // redo (2nd+)
//           [item]() { Repo().remove(item->id); },               // undo
//           [this]() { recarregar(); emit dadosAlterados(); }    // after each op
//       ));
//
//   recarregar();           // widget refreshes itself after the initial op
//   emit dadosAlterados();
//
// Qt calls redo() immediately on push() — the first call is skipped because
// the operation has already been performed by the widget code above.
class LambdaCommand final : public QUndoCommand {
public:
    using Fn = std::function<void()>;

    explicit LambdaCommand(const QString &text, Fn redo, Fn undo,
                           Fn onDone = nullptr,
                           QUndoCommand *parent = nullptr);

    void redo() override;
    void undo() override;

private:
    Fn   m_redo;
    Fn   m_undo;
    Fn   m_onDone;
    bool m_firstRedo = true; // skip the push()-triggered redo()
};
