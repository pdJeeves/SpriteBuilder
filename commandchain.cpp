#include <QTableWidget>
#include "commandchain.h"
#include "spritetable.h"
#include "spritebuilder.h"
#include "imageview.h"

InsertionCommand::InsertionCommand(SpriteTable*, int row) :
	row(row)
{
}

void InsertionCommand::rollForward(SpriteTable* table)
{
	table->insertRow(row);
	table->setCurrentCell(row, table->currentColumn());
}

void InsertionCommand::rollBack(SpriteTable* table)
{
	table->removeRow(row);
}

DeletionCommand::DeletionCommand(SpriteTable*, int row) :
	row(row)
{
}

void DeletionCommand::rollForward(SpriteTable* table)
{
	table->removeRow(row);
}

void DeletionCommand::rollBack(SpriteTable* table)
{
	table->insertRow(row);
	table->setCurrentCell(row, table->currentColumn());
}

SetCommand::SetCommand(SpriteTable*, int row, int column) :
	row(row),
	column(column),
	object(0L)
{
}

SetCommand::SetCommand(SpriteTable* table, int row, int column, const QImage & image) :
	SetCommand(table, row, column)
{
	object = new ImageView(table, row, column);
	object->setImage(image);
}

SetCommand::~SetCommand()
{
	delete object;
}

void SetCommand::rollBack(SpriteTable* table)
{
	auto replace = dynamic_cast<ImageView*>(table->takeItem(row, column));
	if(replace)
	{
		replace->deinitialize();
	}

	if(object)
	{
		object->initialize(table);
		table->setItem(row, column, object);
	}

	object = replace;
}

//just swap them back and forth
void SetCommand::rollForward(SpriteTable* table)
{
	SetCommand::rollBack(table);
}


void RearrangeCommand::rollBack(SpriteTable*table)
{
	for(int i = 0; i < table->rowCount(); i += 4)
	{
		for(int j = 0; j < table->columnCount(); ++j)
		{
			auto a = table->takeItem(i, j);
			auto b = table->takeItem(i+3, j);
			table->setItem(i, j, b);
			table->setItem(i+3, j, a);

			a = table->takeItem(i+1, j);
			b = table->takeItem(i+2, j);
			table->setItem(i+1, j, b);
			table->setItem(i+2, j, a);
		}
	}
}


GroupCommand::~GroupCommand()
{
	for(auto i = begin(); i != end(); ++i)
	{
		delete (*i);
	}
}

void GroupCommand::rollBack(SpriteTable* table)
{
	for(auto i = rbegin(); i != rend(); ++i)
	{
		(*i)->rollBack(table);
	}
}

void GroupCommand::rollForward(SpriteTable* table)
{
	for(auto i = begin(); i != end(); ++i)
	{
		(*i)->rollForward(table);
	}
}

void GroupCommand::intialize(SpriteTable* table)
{
	for(auto i = begin(); i != end(); ++i)
	{
		(*i)->intialize(table);
	}
}

CommandList::CommandList()
{
	itr		= list.begin();
	save	= list.begin();
	builder = 0L;
}

CommandList::~CommandList()
{
	for(auto i = list.begin(); i != list.end(); ++i)
	{
		delete *i;
	}
}

void CommandList::clear()
{
	for(auto i = list.begin(); i != list.end(); ++i)
	{
		delete *i;
	}

	list.clear();
	itr		= list.begin();
	save	= list.begin();

	if(builder)
	{
		builder->updateTitleBar();
	}
}

bool CommandList::dirtyPage()    const
{
	return itr != save;
}

void CommandList::onSave()
{
	save = itr;
}


bool CommandList::canRollBack()    const
{
	return itr != list.begin();
}

bool CommandList::canRollForward() const
{
	return itr != list.end();
}

void CommandList::rollBack(SpriteTable * table)
{
	if(itr != list.begin())
	{
		--itr;
		(*itr)->rollBack(table);
	}
}

void CommandList::rollForward(SpriteTable *table)
{
	if(itr != list.end())
	{
		(*itr)->rollForward(table);
		++itr;
	}
}

void CommandList::push(SpriteTable *table, CommandInterface * it)
{
	if(itr != list.end())
	{
		for(auto i = itr; i != list.end(); ++i)
		{
			delete *i;
		}

		list.erase(itr, list.end());
	}

	list.push_back(it);
	itr = list.end();
	it->intialize(table);

	if(save == list.end())
	{
		--save;
	}

	if(builder)
	{
		builder->updateTitleBar();
	}

	table->resizeColumnsToContents();
	table->resizeRowsToContents();
}
