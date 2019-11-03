#pragma once

#include <fstream>

#include <fbxsdk.h>
#include <DirectXMath.h>
using namespace DirectX;


class FbxLoader 
{
public:
	FbxLoader() {};
	~FbxLoader() {};
	void Test(const char* lFilename);

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