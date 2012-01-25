/*
* thing.h - Sof Game Psi plugin
* Copyright (C) 2010  Aleksey Andreev
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* You can also redistribute and/or modify this program under the
* terms of the Psi License, specified in the accompanied COPYING
* file, as published by the Psi Project; either dated January 1st,
* 2005, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this library; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
*/

#ifndef THING_H
#define THING_H

#include <QtCore>
#include <QDomDocument>


class Thing//: public QObject
{
	//	Q_OBJECT

public:
	enum ToStringFlag {
		ShowNumber = 0x0001,
		ShowName = 0x0002,
		ShowType = 0x0004,
		ShowModif = 0x0008,
		ShowReq = 0x0010,
		ShowUplevel = 0x0020,
		ShowCount = 0x0040,
		ShowAll = ShowNumber | ShowName | ShowType | ShowModif | ShowReq | ShowUplevel | ShowCount
	};
	Thing();
	Thing(const QString);
	~Thing();
	// --
	bool  isEqual(const Thing*) const;
	bool  isValid() const;
	void  setDressed(bool _dressed) {dressed_ = _dressed;};
	int   number() const {return number_;};
	void  setNumber(int);
	QString name() const {return name_;};
	void  setName(QString &);
	int   type() const {return type_;};
	void  setType(int);
	int   count() const {return count_;};
	void  setCount(int);
	int   uplevel() const {return up_level;};
	void  setUplevel(int _level) {up_level = _level;};
	int   price() const {return price_;};
	void  setPrice(int);
	bool  isDressed() const {return dressed_;};
	int   loss() const {return loss_;};
	void  setLoss(int);
	float lossmul() const {return loss_mul;};
	void  setLossmul(float);
	int   protect() const {return protect_;};
	void  setProtect(int);
	float protectmul() const {return protect_mul;};
	void  setProtectmul(float);
	int   force() const {return force_;};
	void  setForce(int);
	float forcemul() const {return force_mul;};
	void  setForcemul(float);
	int   dext() const {return dext_;};
	void  setDext(int);
	float dextmul() const {return dext_mul;};
	void  setDextmul(float);
	int   intell() const {return intell_;};
	void  setIntell(int);
	float intellmul() const {return intell_mul;};
	void  setIntellmul(float);
	QString othermodif() const {return param_str;};
	void  setOthermodif(QString &);
	int   reqlevel() const {return req_level;};
	void  setReqlevel(int _reqlevel) {req_level = _reqlevel;};
	int   reqforce() const {return req_force;};
	void  setReqforce(int _reqforce) {req_force = _reqforce;};
	int   reqdext() const {return req_dext;};
	void  setReqdext(int _reqdext) {req_dext = _reqdext;};
	int   reqintell() const {return req_intell;};
	void  setReqintell(int _reqintell) {req_intell = _reqintell;};
	QString toString(QFlags<enum ToStringFlag>) const;
	static QString paramToStr(float mul, int abs);
	QString toTip() const;
	void  importFromXml(const QDomElement &);
	QDomElement exportToXml(QDomDocument &xmlDoc) const;

private:
	enum ValidStatus {
		Unknow, Valid, Invalid
	};
	mutable ValidStatus valid_status; // Для ленивой проверки на валидность
	int         status;
	int         type_;
	bool        dressed_;
	int         number_;
	QString     name_;
	QString     param_str;
	int         up_level;
	int         count_;
	int         price_;
	int         loss_;
	float       loss_mul;
	int         protect_;
	float       protect_mul;
	int         force_;
	float       force_mul;
	int         dext_;
	float       dext_mul;
	int         intell_;
	float       intell_mul;
	int         req_level;
	int         req_force;
	int         req_dext;
	int         req_intell;

private:
	void init();

};

#endif
