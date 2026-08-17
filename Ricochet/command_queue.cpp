#include "command_queue.hpp"

void CommandQueue::Push(const Command& command)
{
    m_queue.push(Command);
}

Command CommandQueue::Pop()
{
    Command command = m_queue.front();
    m_queue.pop();
    return command;
}

bool CommandQueue::IsEmpty() const
{
    return m_queue.empty();
}