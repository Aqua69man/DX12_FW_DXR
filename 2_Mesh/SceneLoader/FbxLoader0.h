#pragma once

#include <fstream>

#include <fbxsdk.h>
#include <DirectXMath.h>
using namespace DirectX;


class FbxLoader0 
{
public:
	FbxLoader0(const char* outXmlFilepath);
	~FbxLoader0();
	void PrintFbxContent(const char* inFbxFilepath);

protected:
	/* Print the required number of tabs. */
	void PrintTabs();

	/* Return a string-based representation based on the attribute type. */
	FbxString GetAttributeTypeName(FbxNodeAttribute::EType type);

	/* Print an attribute. */
	void PrintAttribute(FbxNodeAttribute* pAttribute);

	/* Print a node, its attributes, and all its children recursively. */
	void PrintNode(FbxNode* pNode);

private:
	/* Tab character ("\t") counter */
	int numTabs = 0;
	std::ofstream m_File;
};