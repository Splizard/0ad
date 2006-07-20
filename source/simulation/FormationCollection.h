//FormationCollection.h
//Andrew aka pyrolink:  ajdecker1022@msn.com

//Nearly identical to EntityTemplateCollection and associates i.e. EntityTemplate, Entity.
//This is the manager of the entity formation "templates"


#ifndef FORMATION_COLLECTION_INCLUDED
#define FORMATION_COLLECTION_INCLUDED

#include <vector>
#include "ps/CStr.h"
#include "ps/Singleton.h"
#include "Formation.h"

#define g_EntityFormationCollection CFormationCollection::GetSingleton()


class CFormationCollection : public Singleton<CFormationCollection>
{
	
	typedef std::map<CStrW, CFormation*> templateMap;
	typedef std::map<CStrW, CStr> templateFilenameMap;
	templateMap m_templates;
	templateFilenameMap m_templateFilenames;
public:
	~CFormationCollection();
	CFormation* getTemplate( const CStrW& formationType );
	int loadTemplates();
	void LoadFile( const char* path );

	// Create a list of the names of all base entities, excluding template_*,
	// for display in ScEd's entity-selection box.
	void getFormationNames( std::vector<CStrW>& names );
};

#endif
