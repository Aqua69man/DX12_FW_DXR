#include "FbxLoader0.h"

FbxLoader0::FbxLoader0(const char* outXmlFilepath)
{
	m_File.open(outXmlFilepath);
}

FbxLoader0::~FbxLoader0()
{
	m_File.close();
}

void FbxLoader0::PrintFbxContent(const char* inFbxFilepath)
{
	// Initialize the SDK manager. This object handles all our memory management.
	FbxManager* lSdkManager = FbxManager::Create();

	// Create the IO settings object.
	FbxIOSettings *ios = FbxIOSettings::Create(lSdkManager, IOSROOT);
	lSdkManager->SetIOSettings(ios);

	// Create an importer using the SDK manager.
	FbxImporter* lImporter = FbxImporter::Create(lSdkManager, "");

	// Use the first argument as the filename for the importer.
	if (!lImporter->Initialize(inFbxFilepath, -1, lSdkManager->GetIOSettings())) {
		m_File << "Call to FbxImporter::Initialize() failed.\n";
		m_File << "Error returned: " << lImporter->GetStatus().GetErrorString() << "\n\n";
		exit(-1);
	}

	// Create a new scene so that it can be populated by the imported file.
	FbxScene* lScene = FbxScene::Create(lSdkManager, "myScene");

	// Import the contents of the file into the scene.
	lImporter->Import(lScene);

	// The file is imported; so get rid of the importer.
	lImporter->Destroy();

	// Print the nodes of the scene and their attributes recursively.
	// Note that we are not printing the root node because it should
	// not contain any attributes.
	FbxNode* lRootNode = lScene->GetRootNode();
	if (lRootNode) {
		for (int i = 0; i < lRootNode->GetChildCount(); i++) {
			PrintNode(lRootNode->GetChild(i));
		}
	}
	// Destroy the SDK manager and all the other objects it was handling.
	lSdkManager->Destroy();
}

/**
 * Print the required number of tabs.
 */
void FbxLoader0::PrintTabs() {
	for (int i = 0; i < numTabs; i++)
		m_File << "\t";
}

/**
 * Return a string-based representation based on the attribute type.
 */
FbxString FbxLoader0::GetAttributeTypeName(FbxNodeAttribute::EType type) {
	switch (type) {
	case FbxNodeAttribute::eUnknown: return "unidentified";
	case FbxNodeAttribute::eNull: return "null";
	case FbxNodeAttribute::eMarker: return "marker";
	case FbxNodeAttribute::eSkeleton: return "skeleton";
	case FbxNodeAttribute::eMesh: return "mesh";
	case FbxNodeAttribute::eNurbs: return "nurbs";
	case FbxNodeAttribute::ePatch: return "patch";
	case FbxNodeAttribute::eCamera: return "camera";
	case FbxNodeAttribute::eCameraStereo: return "stereo";
	case FbxNodeAttribute::eCameraSwitcher: return "camera switcher";
	case FbxNodeAttribute::eLight: return "light";
	case FbxNodeAttribute::eOpticalReference: return "optical reference";
	case FbxNodeAttribute::eOpticalMarker: return "marker";
	case FbxNodeAttribute::eNurbsCurve: return "nurbs curve";
	case FbxNodeAttribute::eTrimNurbsSurface: return "trim nurbs surface";
	case FbxNodeAttribute::eBoundary: return "boundary";
	case FbxNodeAttribute::eNurbsSurface: return "nurbs surface";
	case FbxNodeAttribute::eShape: return "shape";
	case FbxNodeAttribute::eLODGroup: return "lodgroup";
	case FbxNodeAttribute::eSubDiv: return "subdiv";
	default: return "unknown";
	}
}

/**
 * Print an attribute.
 */
void FbxLoader0::PrintAttribute(FbxNodeAttribute* pAttribute) {
	if (!pAttribute) return;

	FbxString typeName = GetAttributeTypeName(pAttribute->GetAttributeType());
	FbxString attrName = pAttribute->GetName();
	PrintTabs();
	// Note: to retrieve the character array of a FbxString, use its Buffer() method.
	m_File << "<attribute type=" << typeName.Buffer() <<  " name=" << attrName.Buffer() << "/>\n";
}

/**
 * Print a node, its attributes, and all its children recursively.
 */
void FbxLoader0::PrintNode(FbxNode* pNode) {
	PrintTabs();
	const char* nodeName = pNode->GetName();
	FbxDouble3 translation = pNode->LclTranslation.Get();
	FbxDouble3 rotation = pNode->LclRotation.Get();
	FbxDouble3 scaling = pNode->LclScaling.Get();

	// Print the contents of the node.
	m_File	<< "<node name="		<< nodeName
			<< " translation='("	<< translation[0]	<< ", " << translation[1]	<< ", " << translation[2]	<< ")'"
			<< " rotation='("		<< rotation[0]		<< ", " << rotation[1]		<< ", " << rotation[2]		<< ")'"
			<< " scaling='("		<< scaling[0]		<< ", " << scaling[1]		<< ", " << scaling[2]		<< ")'"
			<< ">\n";
	numTabs++;

	// Print the node's attributes.
	for (int i = 0; i < pNode->GetNodeAttributeCount(); i++)
		PrintAttribute(pNode->GetNodeAttributeByIndex(i));

	// Recursively print the children.
	for (int j = 0; j < pNode->GetChildCount(); j++)
		PrintNode(pNode->GetChild(j));

	numTabs--;
	PrintTabs();
	m_File << "</node>\n";
}