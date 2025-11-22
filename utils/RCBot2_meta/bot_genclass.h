// Generic class header
/*
 * Stores generic data structures 
 *   - dataNode : single node
 *   - dataStack : LIFO structure, but also has remove functions
 *                 and many more useful functions that vector doesn't have
 *   - dataQueue : FIFO structure, can also be used as LIFO structure
 *                 can get head and tail, and add to front and back
 *                 also neat functions allowing you to remove nodes
 *   - dataUnconstArray : dynamic array, it works :P
 */

/*
 *    This file is part of RCBot.
 *
 *    RCBot by Paul Murphy adapted from Botman's HPB Bot 2 template.
 *
 *    RCBot is free software; you can redistribute it and/or modify it
 *    under the terms of the GNU General Public License as published by the
 *    Free Software Foundation; either version 2 of the License, or (at
 *    your option) any later version.
 *
 *    RCBot is distributed in the hope that it will be useful, but
 *    WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with RCBot; if not, write to the Free Software Foundation,
 *    Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *    In addition, as a special exception, the author gives permission to
 *    link the code of this program with the Half-Life Game Engine ("HL
 *    Engine") and Modified Game Libraries ("MODs") developed by Valve,
 *    L.L.C ("Valve").  You must obey the GNU General Public License in all
 *    respects for all of the code used other than the HL Engine and MODs
 *    from Valve.  If you modify this file, you may extend this exception
 *    to your version of the file, but you are not obligated to do so.  If
 *    you do not wish to do so, delete this exception statement from your
 *    version.
 *
 */
#ifndef __RCBOT_GENERIC_CLASS_H__
#define __RCBOT_GENERIC_CLASS_H__

#include <vector>

//#include "vstdlib/random.h" // for random functions

#include "bot_mtrand.h"


//////////////////////////////////////////////////////////////////////
// GENERIC CLASSES
//#include "vstdlib/random.h" // for random functions
// Typical Node
template <class T>
class dataNode
{
	public:
		dataNode()// constructor
		{
			m_Next = nullptr;
		}

		void _delete ()
		{
			m_Next = nullptr;
		}
		/*
		~dataNode()
		{			
			m_Next = nullptr;
		}
		*/

		T m_NodeData;
		dataNode<T> *m_Next;
};

template <class T>
class dataStack
{
	public:

		void Init ()
		{
			m_Head = nullptr;
		}

		dataStack()// constructor -- Must be public.
		{
			Init();			
		}

		/*~dataStack()
		{
			// Might cause problems with temporary stacks??
			// make sure head is null after finishing with temp stacks!
			this->Destroy();
		}*/

		void _delete ()
		{
			this->Destroy();
		}

		bool IsMember ( const T pObj )
		{
			dataNode<T> *tempNode = m_Head;

			while ( tempNode )
			{
				if ( tempNode->m_NodeData == pObj )
				{
					// dont want program to free tempNode, already used in stack.
					tempNode = nullptr;
					return true;
				}

				tempNode = tempNode->m_Next;
			}

			return false;
		}

		bool RemoveByPointer ( const T *pObj )
		{
			dataNode<T> *tempNode = m_Head;

			if ( m_Head == nullptr)
				return false;

			if ( &m_Head->m_NodeData == pObj )
			{
				m_Head = m_Head->m_Next;

				delete tempNode;

				return true;
			}

			while ( tempNode && tempNode->m_Next )
			{
				if ( &tempNode->m_Next->m_NodeData == pObj )
				{
					dataNode<T>* deleteNode = tempNode->m_Next;

					tempNode->m_Next = tempNode->m_Next->m_Next;

					delete deleteNode;

					// dont want to free tempnode either!
					tempNode = nullptr;					
					deleteNode = nullptr;

					return true;
				}

				tempNode = tempNode->m_Next;
			}

			return false;
		}

		bool Remove ( const T pObj )
		{
			dataNode<T> *tempNode = m_Head;

			if ( m_Head == nullptr)
				return false;

			if ( m_Head->m_NodeData == pObj )
			{
				m_Head = m_Head->m_Next;

				delete tempNode;

				return true;
			}

			while ( tempNode && tempNode->m_Next )
			{
				if ( tempNode->m_Next->m_NodeData == pObj )
				{
					dataNode<T>* deleteNode = tempNode->m_Next;

					tempNode->m_Next = tempNode->m_Next->m_Next;

					delete deleteNode;

					tempNode = nullptr;
					deleteNode = nullptr;

					return true;
				}

				tempNode = tempNode->m_Next;
			}

			return false;
		}

		void Destroy ()
		{
			while ( m_Head )
			{
				dataNode<T>* tempNode = m_Head;

				m_Head = m_Head->m_Next;

				tempNode->_delete();
				delete tempNode;

				tempNode = nullptr;
			}

			m_Head = nullptr;
		}

		bool IsEmpty ()
		{
			return m_Head == nullptr;
		}

		void Push ( const T pObj )
		{
			dataNode<T> *newNode = new dataNode<T>;

			newNode->m_NodeData = pObj;
			newNode->m_Next = m_Head;

			m_Head = newNode;

		}

		T Pop ()
		{
			dataNode<T> *tempNode = m_Head;

			T returnData = tempNode->m_NodeData;

			m_Head = m_Head->m_Next;
			delete tempNode;

			return returnData;
		}

		T ChooseFromStack ()
		{					
			T *l_pTemp;

			try
			{
				l_pTemp = &m_Head->m_NodeData;		
				
				m_Head = m_Head->m_Next;
			}

			catch ( ... )
			{
				// problem
				//CBotGlobals::botMessage(NULL,0,"Bad pointer in stack, (Resetting tasks) Memory may have been lost");

				m_Head = nullptr;
				// return default
				return T();
			}

			return *l_pTemp;
		}

		T *ChoosePointerFromStack ()
		{
			T *l_pTemp;

			try
			{
				l_pTemp = &m_Head->m_NodeData;
				
				m_Head = m_Head->m_Next;
			}

			catch ( ... )
			{
				// problem
				//BugMessage(NULL,"Bad pointer in stack, (Resetting tasks) Memory may have been lost");

				m_Head = nullptr;
				// return default
				return nullptr;
			}

			return l_pTemp;
		}

		// Returns a pointer to the DETAILS (m_NodeData)
		// in the HEAD.
		T *GetHeadInfoPointer ()
		{
			if ( m_Head )
			{
				return &m_Head->m_NodeData;
			}

			return nullptr;
		}

	private:

		dataNode<T> *m_Head;
};
/*
template <class T>
class dataUnconstArray
{
	public:
		
		dataUnconstArray( )
		{
			this->Init();
		}
		
		~dataUnconstArray()
		{
			this->Clear();
		}

		inline Size ()
		{
			return m_iArrayMax;
		}

		void Remove ( T obj )
		{
			if ( m_pArray == NULL )
				return;

			int i = 0;
			int iRem = -1;

			while ( (i < m_iArrayMax) && (iRem == -1) )
			{
				if ( m_pArray[i] == obj )
					iRem = i;
				else
					i++;
			}

			int iNewSize = m_iArrayMax-1; // smaller array now

			if ( iNewSize <= 0 ) // not valid anymore
			{
				Clear();
				return;
			}

			T *temp = new T[iNewSize];
			int n = 0; // temp count

			for ( i = 0; i < m_iArrayMax; i ++ )
			{
				if ( i != iRem ) // we lose this one
					temp[n++] = m_pArray[i];
			}

			delete m_pArray;//free(m_pArray);
			m_pArray = temp;
			m_iArrayMax = iNewSize;
		}

		inline void Destroy ()
		{
			delete m_pArray;			
		}

		inline void Init ()
		{
			m_pArray = NULL;
			m_iArrayMax = -1;
		}

		inline bool IsEmpty ()
		{
			return (m_pArray == NULL);
		}

		

		inline void Clear ()
		{
			this->Destroy();
			this->Init();
		}

		T Random ()
		{
			return m_pArray[RANDOM_INT(0,m_iArrayMax-1)];//RandomInt(0,m_iArrayMax-1)];
		}

		void Add ( const T pObj )
		{
			if ( m_pArray == NULL )
			{
				m_pArray = new T;//(T*)malloc(sizeof(T));
				m_pArray[0] = pObj;
				m_iArrayMax = 1;
			}
			else
			{		
				//m_pArray = (T*)realloc(m_pArray,sizeof(T)*(m_iArrayMax+1));
				//m_pArray[m_iArrayMax] = pObj;
				//m_iArrayMax++;
				
				T *temp = new T[m_iArrayMax+1];

				memcpy(temp,m_pArray,sizeof(T)*m_iArrayMax);

				temp[m_iArrayMax] = pObj;
				m_iArrayMax++;

				delete m_pArray;

				m_pArray = temp;

				//m_pArray = (T*)realloc(m_pArray,sizeof(T) * (m_iArrayMax+1));
				//m_pArray[m_iArrayMax++] = pObj;
			}
		}
		


		T ReturnValueFromIndex ( int iIndex )
		{
			assert(m_pArray != NULL);
			assert(iIndex >= 0);
			assert(iIndex < m_iArrayMax);

			if ( ( m_pArray != NULL ) && ( (iIndex >= 0) && (iIndex < m_iArrayMax) ) )
				return m_pArray[iIndex];

			//CBotGlobals::botMessage(NULL,0,"dataUnconstArray::ReturnValueFromIndex() Error !");

			// return first array index
			return T(NULL);
		}

		bool IsMember ( T Obj )
		{
			int i;

			if ( m_pArray == NULL )
				return false;

			for ( i = 0; i < m_iArrayMax; i ++ )
			{
				if ( m_pArray[i] == Obj )
					return true;
			}

			return false;
		}

		int m_iArrayMax;
		
	protected:
		T *m_pArray;		
};*/
/*
template <class T>
class dataUnconstArray
{
	public:
		
		dataUnconstArray( )
		{
			this->Init();
		}
		
		~dataUnconstArray()
		{
			this->Clear();
		}

		inline Size ()
		{
			return m_iArrayMax;
		}

		void Remove ( T obj )
		{
			if ( m_pArray == NULL )
				return;

			int i = 0;
			int iRem = -1;

			while ( (i < m_iArrayMax) && (iRem == -1) )
			{
				if ( m_pArray[i] == obj )
					iRem = i;
				else
					i++;
			}

			int iNewSize = m_iArrayMax-1; // smaller array now

			if ( iNewSize <= 0 ) // not valid anymore
			{
				Clear();
				return;
			}

			T *temp = new T[iNewSize];
			int n = 0; // temp count

			for ( i = 0; i < m_iArrayMax; i ++ )
			{
				if ( i != iRem ) // we lose this one
					temp[n++] = m_pArray[i];
			}

			delete m_pArray;
			m_pArray = temp;
			m_iArrayMax = iNewSize;
		}

		inline void Destroy ()
		{
			delete m_pArray;
			m_pArray = NULL;
			m_iArrayMax = -1;
		}

		inline void Init ()
		{
			m_pArray = NULL;
			m_iArrayMax = -1;
		}

		inline bool IsEmpty ()
		{
			return (m_pArray == NULL);
		}

		inline void Clear ()
		{
			this->Destroy();
			this->Init();
		}

		T Random ()
		{
			return m_pArray[RANDOM_INT(0,m_iArrayMax-1)];
		}

		void Add ( const T pObj )
		{
			if ( m_pArray == NULL )
			{
				m_pArray = new T;//(T*)malloc(sizeof(T));
				m_pArray[0] = pObj;
				m_iArrayMax = 1;
			}
			else
			{			
				T *temp = new T[m_iArrayMax+1];

				memcpy(temp,m_pArray,sizeof(T)*m_iArrayMax);

				temp[m_iArrayMax] = pObj;
				m_iArrayMax++;

				delete m_pArray;

				m_pArray = temp;
			}
		}

		T ReturnValueFromIndex ( int iIndex )
		{
			assert(m_pArray != NULL);
			assert(iIndex >= 0);
			assert(iIndex < m_iArrayMax);

			if ( ( m_pArray != NULL ) && ( (iIndex >= 0) && (iIndex < m_iArrayMax) ) )
				return m_pArray[iIndex];

			//CBotGlobals::botMessage(NULL,0,"dataUnconstArray::ReturnValueFromIndex() Error !");

			// return first array index
			return T(NULL);
		}

		bool IsMember ( T Obj )
		{
			int i;

			if ( m_pArray == NULL )
				return false;

			for ( i = 0; i < m_iArrayMax; i ++ )
			{
				if ( m_pArray[i] == Obj )
					return true;
			}

			return false;
		}

		int m_iArrayMax;
		
	protected:
		T *m_pArray;		
};
*/
template <class T>
class dataUnconstArray
{
	public:
		
		dataUnconstArray( )
		{
			this->Init();
		}
		
		~dataUnconstArray()
		{
			this->Clear();
		}

		int Size ()
		{
			return array.size();
		}

		void Remove ( T obj )
		{
			if ( array.size() == 0  )
				return;
#if defined(__linux__) && defined(_DEBUG)
			//SAFE REMOVE - SLOW
			std::vector<T> newVec;

			for ( unsigned i = 0; i < array.size(); i ++ )
			{
				if ( array[i] != obj )
					newVec.emplace_back(array[i]);
			}

			array.clear();
			array = newVec;
#elif defined(__linux__) && !defined(_DEBUG)
			typename std::vector<T> ::iterator it;
			for ( it = array.begin(); it != array.end(); )
			{
				if ( *it == obj )
				{
					it = array.erase(it);
					return;
				}
				else
					++ it;
			}
#elif defined(_DEBUG)
			//SAFE REMOVE - SLOW
			std::vector<T> newVec;

			for ( unsigned i = 0; i < array.size(); i ++ )
			{
				if ( array[i] != obj )
					newVec.emplace_back(array[i]);
			}

			array.clear();
			array = newVec;
#else
			typename std::vector<T> ::iterator it;

			for ( it = array.begin(); it != array.end(); )
			{
				if ( *it == obj )
				{
					it = array.erase(it);
					return;
				}
				++ it;
			}
#endif
		}

		void Destroy ()
		{
			array.clear();
		}

		void Init ()
		{
			array.clear();
		}

		bool IsEmpty ()
		{
			return array.empty();
		}

		void Clear ()
		{
			this->Destroy();
			this->Init();
		}

		//static int RandomInteger ( int min, int max );

		T Random ()
		{
			//return array[RANDOM_INT(0,array.size()-1)];
			//return array[RandomInteger(0,array.size()-1)];
			return array[randomInt(0,array.size()-1)];
		}

		void Add ( const T pObj )
		{
			array.emplace_back(pObj);
		}

		T ReturnValueFromIndex ( int iIndex )
		{
			return array[iIndex];
		}

		T *ReturnPointerFromIndex ( int iIndex )
		{
			return &array[iIndex];
		}

		bool IsMember ( T Obj )
		{
			for ( unsigned i = 0; i < array.size(); i ++ )
			{
				if ( array[i] == Obj )
					return true;
			}

			return false;
		}

		T operator [] ( unsigned iIndex )
		{
			return array[iIndex];
		}
	private:
		std::vector<T> array;
};

template <class T>
class dataQueue
{
	public:
		dataQueue()// constructor -- Must be public.
		{
			this->Init();
		}

		// explicit delete function only now..
		void _delete ()
		{
			this->Destroy();
		}

		void Init ()
		{
			m_Head = nullptr;
			m_Tail = nullptr;
		}

		void Destroy ()
		{
			dataNode<T> *tempNode = nullptr;

			while ( m_Head )
			{
				tempNode = m_Head;

				m_Head = m_Head->m_Next;

				tempNode->_delete();
				delete tempNode;

				tempNode = nullptr;
			}

			m_Head = nullptr;
			m_Tail = nullptr;
		}

		bool IsEmpty ()
		{
			return m_Head == nullptr ||m_Tail == nullptr;
		}

		void AddFront ( const T &pObj )
		{
			dataNode<T> *newNode = new dataNode<T>;					

			newNode->m_NodeData = pObj;

			if ( m_Head == nullptr)
			{
				m_Tail = newNode;
				m_Head = newNode;
			}
			else
			{				
				newNode->m_Next = m_Head;
				m_Head = newNode;
			}
		}

		void Add ( const T &pObj )
		{
			dataNode<T> *newNode = new dataNode<T>;
			newNode->m_NodeData = pObj;

			if ( IsEmpty() )
			{
				//newNode->m_Next = m_Head;

				m_Head = newNode;
				m_Tail = newNode;
			}
			else
			{
				m_Tail->m_Next = newNode;
				m_Tail = newNode;
			}
		}

		T GetFrontInfo ()
		{
			return m_Head->m_NodeData;
		}

		T *GetFrontPointer ()
		{
			return &m_Head->m_NodeData;
		}

		T ChooseFrom ()
		{
			T *l_pTemp;

			try
			{
				l_pTemp = &m_Head->m_NodeData;

				m_Head = m_Head->m_Next;
			}

			catch ( ... )
			{
				// problem
				//BugMessage(NULL,"Bad pointer in queue, (Resetting queue) Memory may have been lost");

				m_Head = nullptr;
				m_Tail = nullptr;
				// return default
				return T();
			}

			return *l_pTemp;
		}

		T *ChoosePointerFrom ()
		{
			T *l_pTemp;

			try
			{
				l_pTemp = &m_Head->m_NodeData;
				
				m_Head = m_Head->m_Next;
			}

			catch ( ... )
			{
				// problem
				//BugMessage(NULL,"Bad pointer in queue, (Resetting tasks) Memory may have been lost");

				m_Head = nullptr;
				m_Tail = nullptr;
				// return default
				return nullptr;
			}

			return l_pTemp;
		}

		void RemoveFront ()
		{
			if ( m_Head == nullptr)
			{
				// just set tail to null incase
				m_Tail = nullptr;
				// already empty
				return;
			}
			
			try
			{				
				dataNode<T>* tempNode = m_Head;
				
				if ( m_Tail == m_Head )
				{
					m_Tail = nullptr;
					m_Head = nullptr;
				}
				else 
					m_Head = m_Head->m_Next;
				
				tempNode->_delete();
				delete tempNode;
				
			}
			
			catch ( ... )
			{
				m_Head = nullptr;
				m_Tail = nullptr;

//				BugMessage(NULL,"Bad pointer in queue, (Resetting queue) Memory may have been lost");
			}
		}

		bool IsMember ( const T pObj )
		{
			dataNode<T> *tempNode = m_Head;

			while ( tempNode )
			{
				if ( tempNode->m_NodeData == pObj )
				{
					// dont want program to free tempNode, already used in stack.
					tempNode = nullptr;

					return true;
				}

				tempNode = tempNode->m_Next;
			}

			return false;
		}

		bool Remove ( const T pObj )
		{
			dataNode<T> *tempNode = m_Head;

			if ( m_Head == nullptr)
				return false;

			if ( m_Head->m_NodeData == pObj )
			{
				if ( m_Head == m_Tail )
				{
					m_Tail = nullptr;
					m_Head = nullptr;
				}
				else
				{
					m_Head = m_Head->m_Next;
				}

				tempNode->_delete();
				delete tempNode;

				tempNode = nullptr;

				return true;
			}

			while ( tempNode && tempNode->m_Next )
			{
				if ( tempNode->m_Next->m_NodeData == pObj )
				{
					dataNode<T>* deleteNode = tempNode->m_Next;

					if ( deleteNode == m_Tail )
					{
						m_Tail = tempNode;
						tempNode->m_Next = nullptr;
					}
					else
						tempNode->m_Next = deleteNode->m_Next;

					delete deleteNode;
					
					tempNode = nullptr;
					deleteNode = nullptr;

					return true;
				}

				tempNode = tempNode->m_Next;
			}

			return false;
		}

		bool RemoveByPointer ( const T *pObj )
		{
			dataNode<T> *tempNode = m_Head;

			if ( m_Head == nullptr)
				return false;

			if ( &m_Head->m_NodeData == pObj )
			{
				if ( m_Head != m_Tail )
				{
					m_Head = m_Head->m_Next;
				}
				else
				{
					m_Head = nullptr;
					m_Tail = nullptr;
				}

				delete tempNode;
				tempNode = nullptr;

				return true;
			}

			while ( tempNode && tempNode->m_Next )
			{
				if ( &tempNode->m_Next->m_NodeData == pObj )
				{
					dataNode<T>* deleteNode = tempNode->m_Next;

					if ( deleteNode == m_Tail )
					{
						m_Tail = tempNode;
						tempNode->m_Next = nullptr;
					}
					else
						tempNode->m_Next = deleteNode->m_Next;

					deleteNode->_delete();
					delete deleteNode;

					tempNode = nullptr;
					deleteNode = nullptr;

					return true;
				}

				tempNode = tempNode->m_Next;
			}

			return false;
		}

	private:

		dataNode<T> *m_Head;
		dataNode<T> *m_Tail;

};

#endif