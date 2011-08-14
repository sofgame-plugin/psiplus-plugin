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
		bool  isEqual(Thing*);
		bool  isValid();
		void  setDressed(bool);
		int   number();
		void  setNumber(int);
		QString  name();
		void  setName(QString);
		int   type();
		void  setType(int);
		int   count();
		void  setCount(int);
		int   uplevel();
		void  setUplevel(int);
		int   price();
		void  setPrice(int);
		bool  isDressed();
		int   loss();
		void  setLoss(int);
		float lossmul();
		void  setLossmul(float);
		int   protect();
		void  setProtect(int);
		float protectmul();
		void  setProtectmul(float);
		int   force();
		void  setForce(int);
		float forcemul();
		void  setForcemul(float);
		int   dext();
		void  setDext(int);
		float dextmul();
		void  setDextmul(float);
		int   intell();
		void  setIntell(int);
		float intellmul();
		void  setIntellmul(float);
		QString othermodif();
		void  setOthermodif(QString);
		int   reqlevel();
		void  setReqlevel(int);
		int   reqforce();
		void  setReqforce(int);
		int   reqdext();
		void  setReqdext(int);
		int   reqintell();
		void  setReqintell(int);
		QString toString(QFlags<enum ToStringFlag>);
		void  importFromXml(const QDomElement &);
		void  exportToXml(QDomDocument*, QDomElement*);

	private:
		enum ValidStatus {
			Unknow, Valid, Invalid
		};
		ValidStatus valid_status;
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
		void init();

	protected:


};

#endif
