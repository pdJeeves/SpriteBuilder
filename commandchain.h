#ifndef COMMANDCHAIN_H
#define COMMANDCHAIN_H
#include <vector>
#include <QImage>

class SpriteBuilder;
class SpriteTable;
class ImageView;

struct CommandInterface
{
	virtual ~CommandInterface() {}
	virtual void rollBack(SpriteTable*) {}
	virtual void rollForward(SpriteTable*) {}
	virtual void intialize(SpriteTable* it) { rollForward(it); }
};

class InsertionCommand  : public CommandInterface
{
	const int row;

public:
	InsertionCommand(SpriteTable*, int row);

	void rollBack(SpriteTable*);
	void rollForward(SpriteTable*);
};

class DeletionCommand  : public CommandInterface
{
	const int row;

public:
	DeletionCommand(SpriteTable*, int row);

	void rollBack(SpriteTable*);
	void rollForward(SpriteTable*);
};

class SetCommand  : public CommandInterface
{
	const int row, column;
	ImageView * object;

public:
	SetCommand(SpriteTable*, int row, int column);
	SetCommand(SpriteTable*, int row, int column, const QImage & image);
	~SetCommand();

	void rollBack(SpriteTable*);
	void rollForward(SpriteTable*);
};

class RearrangeCommand  : public CommandInterface
{
public:

	void rollBack(SpriteTable*);
	void rollForward(SpriteTable*table) { RearrangeCommand::rollBack(table); }
};

class GroupCommand : public CommandInterface, public std::vector<CommandInterface *>
{
public:
	~GroupCommand();

	void rollBack(SpriteTable*);
	void rollForward(SpriteTable*);
	void intialize(SpriteTable*);
};


class CommandList
{
typedef std::list<CommandInterface *> List;
private:
	List		    list;
	List::iterator  itr;
	List::iterator  save;

public:
	SpriteBuilder * builder;

	CommandList();
	~CommandList();

	bool dirtyPage()      const;
	void onSave();

	bool canRollBack()    const;
	bool canRollForward() const;

	void rollBack(SpriteTable *table);
	void rollForward(SpriteTable *table);

	void clear();

	void push(SpriteTable *table, CommandInterface *);
};
#endif // COMMANDCHAIN_H
