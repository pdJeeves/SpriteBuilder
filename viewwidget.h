#ifndef VIEWWIDGET_H
#define VIEWWIDGET_H
#include <QOpenGLWidget>
#include <QWidget>
#include <QTime>

class SpriteBuidler;

class ViewWidget : public QWidget
{
typedef QWidget super;
	Q_OBJECT
private:
	QTime			time;
	SpriteBuidler * window;
	QPoint			last_pos;

public:
	explicit ViewWidget(QWidget *parent = 0);

	void setOwner(SpriteBuidler * o);

	void mousePressEvent		(QMouseEvent * event)	Q_DECL_OVERRIDE;
	void mouseReleaseEvent		(QMouseEvent * event)	Q_DECL_OVERRIDE;
	void mouseDoubleClickEvent	(QMouseEvent * event)	Q_DECL_OVERRIDE;
	void mouseMoveEvent			(QMouseEvent * event)	Q_DECL_OVERRIDE;

	void wheelEvent				(QWheelEvent * event)   Q_DECL_OVERRIDE;
	void keyPressEvent			(QKeyEvent * event)		Q_DECL_OVERRIDE;
	void keyReleaseEvent		(QKeyEvent * event)		Q_DECL_OVERRIDE;

	bool event					(QEvent *event)			Q_DECL_OVERRIDE;
	void paintEvent				(QPaintEvent * event)	Q_DECL_OVERRIDE;

public slots:
	void ShowContextMenu		(const QPoint & pos);

};

#endif
