#ifndef APP_H
#define APP_H

#include "stdafx.h"

class MyApp : public wxApp
{
	public:
		virtual bool OnInit();
	private:
		wxLocale m_locale;
};

#endif // APP_H
