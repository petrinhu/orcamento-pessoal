#include "UndoCommands.h"

LambdaCommand::LambdaCommand(const QString &text, Fn redo, Fn undo,
                              Fn onDone, QUndoCommand *parent)
    : QUndoCommand(text, parent)
    , m_redo(std::move(redo))
    , m_undo(std::move(undo))
    , m_onDone(std::move(onDone))
{}

void LambdaCommand::redo()
{
    if (m_firstRedo) { m_firstRedo = false; return; }
    if (m_redo) m_redo();
    if (m_onDone) m_onDone();
}

void LambdaCommand::undo()
{
    if (m_undo) m_undo();
    if (m_onDone) m_onDone();
}
