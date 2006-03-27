/* 
 *	Copyright (C) 2003-2006 Gabest
 *	http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "Node.h"
#include "NodeFactory.h"
#include "Exception.h"
#include "Split.h"

#include <math.h>

namespace ssf
{
	Node::Node(const NodeFactory* pnf, CStringW name)
		: m_pnf(pnf)
		, m_type('?')
		, m_name(name)
		, m_priority(PNormal)
		, m_parent(NULL)
	{
		ASSERT(m_pnf);
	}

	void Node::AddTail(Node* pNode)
	{
		if(POSITION pos = m_nodes.Find(pNode)) // TODO: slow
		{
			m_nodes.MoveToTail(pos);
			return;
		}

		m_nodes.AddTail(pNode);
		m_name2node[pNode->m_name] = pNode;
	}

	bool Node::IsNameUnknown()
	{
		return m_name.IsEmpty() || !!iswdigit(m_name[0]);
	}

	bool Node::IsTypeUnknown()
	{
		return m_type.IsEmpty() || m_type == '?';
	}

	bool Node::IsType(CStringW type)
	{
		return m_type == type;
	}

	void Node::GetChildDefs(CAtlList<Definition*>& l, LPCWSTR type, bool fFirst)
	{
		CAtlList<Definition*> rdl[3];

		if(fFirst)
		{
			if(Definition* pDef = m_pnf->GetDefByName(m_type))
			{
				pDef->GetChildDefs(rdl[pDef->m_priority], type, false);
			}
		}

		POSITION pos = m_nodes.GetHeadPosition();
		while(pos)
		{
			if(Node* pNode = m_nodes.GetNext(pos))
			{
				pNode->GetChildDefs(rdl[pNode->m_priority], type, false);
			}
		}

		for(int i = 0; i < sizeof(rdl)/sizeof(rdl[0]); i++)
		{
			l.AddTailList(&rdl[i]);
		}
	}

	// Reference

	Reference::Reference(const NodeFactory* pnf, CStringW name)
		: Node(pnf, name)
	{
	}

	Reference::~Reference()
	{
	}

	void Reference::GetChildDefs(CAtlList<Definition*>& l, LPCWSTR type, bool fFirst)
	{
		CAtlList<Definition*> rdl[3];

		POSITION pos = m_nodes.GetHeadPosition();
		while(pos)
		{
			if(Definition* pDef = dynamic_cast<Definition*>(m_nodes.GetNext(pos)))
			{
				if(!type || pDef->m_type == type) // TODO: faster lookup
				{
					rdl[pDef->m_priority].AddTail(pDef);
				}
			}
		}

		for(int i = 0; i < sizeof(rdl)/sizeof(rdl[0]); i++)
		{
			l.AddTailList(&rdl[i]);
		}
	}

	void Reference::Dump(NodePriority priority, int level, bool fLast)
	{
		if(m_priority < priority) return;

		CString tabs(' ', level*4);

		// TRACE(tabs + _T("\n") + tabs + _T(" {\n"));
		TRACE(L" {\n");

		POSITION pos = m_nodes.GetHeadPosition();
		while(pos)
		{
			if(Definition* pDef = dynamic_cast<Definition*>(m_nodes.GetNext(pos)))
			{
				pDef->Dump(priority, level + 1, pos == NULL);
			}
		}

		TRACE(tabs + '}');
	}

	// Definition

	Definition::Definition(const NodeFactory* pnf, CStringW name)
		: Node(pnf, name)
		, m_status(node)
		, m_autotype(false)
	{
	}

	Definition::~Definition()
	{
		RemoveFromCache();
	}

	bool Definition::IsVisible(Definition* pDef)
	{
		Node* pNode = m_parent;

		while(pNode)
		{
			if(pNode->m_name2node.Lookup(pDef->m_name))
			{
				return true;
			}

			pNode = pNode->m_parent;
		}

		return false;
	}

	void Definition::AddTail(Node* pNode)
	{
//		if(Reference* pRef = dynamic_cast<Reference*>(pNode))
		{
			ASSERT(m_status == node);

			m_status = node;

			if(IsTypeUnknown() && !pNode->IsTypeUnknown())
			{
				m_type = pNode->m_type; 
				m_autotype = true;
			}

			RemoveFromCache(pNode->m_type);
		}

		__super::AddTail(pNode);
	}

	Definition& Definition::operator[] (LPCWSTR type) 
	{
		Definition* pRetDef = NULL;
		if(m_type2def.Lookup(type, pRetDef))
			return *pRetDef;

		pRetDef = new Definition(m_pnf, L"");
		pRetDef->m_priority = PLow;
		pRetDef->m_type = type;
		m_type2def[type] = pRetDef;

		CAtlList<Definition*> l;
		GetChildDefs(l, type);

		while(!l.IsEmpty())
		{
			Definition* pDef = l.RemoveHead();

			pRetDef->m_priority = pDef->m_priority;
			pRetDef->m_parent = pDef->m_parent;

			if(pDef->IsValue())
			{
				pRetDef->SetAsValue(pDef->m_status, pDef->m_value, pDef->m_unit);
			}
			else
			{
				pRetDef->m_status = node; 
				pRetDef->m_nodes.AddTailList(&pDef->m_nodes);
			}
		}

		return *pRetDef;
	}

	void Definition::RemoveFromCache(LPCWSTR type)
	{
		if(!type)
		{
			POSITION pos = m_type2def.GetStartPosition();
			while(pos) delete m_type2def.GetNextValue(pos);
		}
		else if(CAtlStringMapW<Definition*>::CPair* p = m_type2def.Lookup(type))
		{
			delete p->m_value;
			m_type2def.RemoveKey(type);
		}
	}

	bool Definition::IsValue(status_t s)
	{
		return s ? m_status == s : m_status != node;
	}

	void Definition::SetAsValue(status_t s, CStringW v, CStringW u)
	{
		ASSERT(s != node);

		m_nodes.RemoveAll();
		m_name2node.RemoveAll();

		m_status = s;

		m_value = v;
		m_unit = u;
	}

	void Definition::SetAsNumber(CStringW v, CStringW u)
	{
		SetAsValue(number, v, u);

		Number<double> n;
		GetAsNumber(n); // will throw an exception if not a number
	}

	template<class T> 
	void Definition::GetAsNumber(Number<T>& n, CAtlStringMapW<T>* n2n)
	{
		CStringW str = m_value;
		str.Replace(L" ", L"");

		n.value = 0;
		n.unit = m_unit;
		n.sign = 0;

		if(n2n)
		{
			if(m_status == node) throw Exception(_T("expected value type"));

			if(CAtlStringMapW<T>::CPair* p = n2n->Lookup(str))
			{
				n.value = p->m_value;
				return;
			}
		}

		if(m_status != number) throw Exception(_T("expected number"));

		n.sign = str.Find('+') == 0 ? 1 : str.Find('-') == 0 ? -1 : 0;
		str.TrimLeft(L"+-");

		if(str.Find(L"0x") == 0)
		{
			if(n.sign) throw Exception(_T("hex values must be unsigned"));

			n.value = (T)wcstoul(str.Mid(2), NULL, 16);
		}
		else
		{
			Split sa(':', str);
			Split sa2('.', sa ? sa[sa-1] : L"");

			if(sa == 0 || sa2 == 0 || sa2 > 2) throw Exception(_T("invalid number"));

			double d = 0;
			for(size_t i = 0; i < sa; i++) {d *= 60; d += wcstoul(sa[i], NULL, 10);}
			if(sa2 > 1) d += (double)wcstoul(sa2[1], NULL, 10) / pow((float)10, sa2[1].GetLength());

			if(n.unit == L"ms") {d /= 1000; n.unit = L"s";}
			else if(n.unit == L"m") {d *= 60; n.unit = L"s";}
			else if(n.unit == L"h") {d *= 3600; n.unit = L"s";}

			n.value = (T)d;

			if(n.sign) n.value *= n.sign;
		}
	}

	void Definition::GetAsString(CStringW& str)
	{
		if(m_status == node) throw Exception(_T("expected value type"));

		str = m_value; 
	}

	void Definition::GetAsNumber(Number<int>& n, CAtlStringMapW<int>* n2n) {return GetAsNumber<int>(n, n2n);}
	void Definition::GetAsNumber(Number<DWORD>& n, CAtlStringMapW<DWORD>* n2n) {return GetAsNumber<DWORD>(n, n2n);}
	void Definition::GetAsNumber(Number<double>& n, CAtlStringMapW<double>* n2n) {return GetAsNumber<double>(n, n2n);}

	void Definition::GetAsBoolean(bool& b)
	{
		static CAtlStringMapW<bool> s2b;

		if(s2b.IsEmpty())
		{
			s2b[L"true"] = true;
			s2b[L"on"] = true;
			s2b[L"yes"] = true;
			s2b[L"1"] = true;
			s2b[L"false"] = false;
			s2b[L"off"] = false;
			s2b[L"no"] = false;
			s2b[L"0"] = false;
		}

		if(!s2b.Lookup(m_value, b)) // m_status != boolean && m_status != number || 
		{
			throw Exception(_T("expected boolean"));
		}
	}

	bool Definition::GetAsTime(Time& t, CAtlStringMapW<double>& offset, CAtlStringMapW<double>* n2n, int default_id)
	{
		Definition& time = (*this)[L"time"];

		CStringW id;
		if(time[L"id"].IsValue()) id = time[L"id"];
		else id.Format(L"%d", default_id);

		double scale = time[L"scale"].IsValue() ? time[L"scale"] : 1.0;

		if(time[L"start"].IsValue() && time[L"stop"].IsValue())
		{
			time[L"start"].GetAsNumber(t.start, n2n);
			time[L"stop"].GetAsNumber(t.stop, n2n);

			if(t.start.unit.IsEmpty()) t.start.value *= scale;
			if(t.stop.unit.IsEmpty()) t.stop.value *= scale;

			double o = 0;
			offset.Lookup(id, o);

			if(t.start.sign != 0) t.start.value = o + t.start.value;
			if(t.stop.sign != 0) t.stop.value = t.start.value + t.stop.value;

			offset[id] = t.stop.value;

			return true;
		}

		return false;
	}

	Definition::operator LPCWSTR()
	{
		CStringW str;
		GetAsString(str);
		return str;
	}

	Definition::operator double()
	{
		double d;
		GetAsNumber(d);
		return d;
	}

	Definition::operator bool()
	{
		bool b;
		GetAsBoolean(b);
		return b;
	}

	void Definition::Dump(NodePriority priority, int level, bool fLast)
	{
		if(m_priority < priority) return;

		CString tabs(' ', level*4);

		CString str = tabs;
		if(m_priority == PLow) str += '*';
		else if(m_priority == PHigh) str += '!';
		if(!IsTypeUnknown() && !m_autotype) str += CString(m_type);
		if(!IsNameUnknown()) str += '#' + CString(m_name);
		str += ':';
		TRACE(_T("%s"), str);

		if(!m_nodes.IsEmpty())
		{
			POSITION pos = m_nodes.GetHeadPosition();
			while(pos)
			{
				Node* pNode = m_nodes.GetNext(pos);

				if(Reference* pRef = dynamic_cast<Reference*>(pNode))
				{
					pRef->Dump(priority, level, fLast);
				}
				else 
				{
					ASSERT(!pNode->IsNameUnknown());
					TRACE(_T(" %s"), CString(pNode->m_name));
				}
			}

			TRACE(_T(";\n"));

			if(!fLast && (!m_nodes.IsEmpty() || level == 0)) TRACE(_T("\n"));
		}
		else if(m_status == string)
		{
			CString str = CString(m_value);
			str.Replace(_T("\""), _T("\\\""));
			TRACE(_T(" \"%s\";\n"), str);
		}
		else if(m_status == number)
		{
			CString str = CString(m_value);
			if(!m_unit.IsEmpty()) str += CString(m_unit);
			TRACE(_T(" %s;\n"), str);
		}
		else if(m_status == boolean)
		{
			TRACE(_T(" %s;\n"), CString(m_value));
		}
		else if(m_status == block)
		{
			TRACE(_T(" {%s};\n"), CString(m_value));
		}
		else
		{
			TRACE(_T(" null;\n"));
		}
	}
}