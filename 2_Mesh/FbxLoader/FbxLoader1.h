#pragma once

#include <fbxsdk.h>
#include <assert.h>

#include <vector>
#include <set>
//#include <cmath>

#include <DirectXMath.h>
using namespace DirectX;

#define INDEXED_DRAW_SUPPORTED

// Vertex data for a colored cube.
struct VertexPosColor
{
	XMFLOAT3 Position;
	XMFLOAT3 Color;

	bool operator== (const VertexPosColor & v2) const {
		return (Position.x == v2.Position.x &&
				Position.y == v2.Position.y &&
				Position.z == v2.Position.z);
	}
};
typedef std::pair<VertexPosColor, uint16_t> viPair;


struct CmpClass // class comparing vertices in the set
{
	const float EPS = 0.0; //0.001;

	bool operator() (const viPair& p1, const viPair& p2) const
	{
		if (fabs(p1.first.Position.x - p2.first.Position.x) > EPS) return p1.first.Position.x < p2.first.Position.x;
		if (fabs(p1.first.Position.y - p2.first.Position.y) > EPS) return p1.first.Position.y < p2.first.Position.y;
		if (fabs(p1.first.Position.z - p2.first.Position.z) > EPS) return p1.first.Position.z < p2.first.Position.z;
		return false;
	}
};
std::set<viPair, CmpClass> setVertInd;


FbxManager* g_pFbxSdkManager = nullptr;

#define USE_OPTIMIZED_SEARCH
#ifdef USE_OPTIMIZED_SEARCH
// Loads the FBX file and retrives vertices and indices - loading is too slow (TODO)
bool LoadFBX(const char * fbxFilePath, std::vector<VertexPosColor> * pOutVertices, std::vector<uint16_t> * pOutIndices)
{
	if (g_pFbxSdkManager == nullptr)
	{
		g_pFbxSdkManager = FbxManager::Create();

		FbxIOSettings* pIOsettings = FbxIOSettings::Create(g_pFbxSdkManager, IOSROOT);
		g_pFbxSdkManager->SetIOSettings(pIOsettings);
	}

	FbxImporter* pImporter = FbxImporter::Create(g_pFbxSdkManager, "");
	FbxScene* pFbxScene = FbxScene::Create(g_pFbxSdkManager, "");

	bool bSuccess = pImporter->Initialize(fbxFilePath, -1, g_pFbxSdkManager->GetIOSettings());
	if (!bSuccess) return false;

	bSuccess = pImporter->Import(pFbxScene);
	if (!bSuccess) return false;

	pImporter->Destroy();

	// ----------------------------- Convert Axis
	// Convert Axis System to what is used in this example, if needed
	FbxAxisSystem SceneAxisSystem = pFbxScene->GetGlobalSettings().GetAxisSystem();
	FbxAxisSystem OurAxisSystem(FbxAxisSystem::eYAxis, FbxAxisSystem::eParityOdd, FbxAxisSystem::eRightHanded);
	if (SceneAxisSystem != OurAxisSystem)
	{
		OurAxisSystem.ConvertScene(pFbxScene);
	}

	// Convert Unit System to what is used in this example, if needed
	FbxSystemUnit SceneSystemUnit = pFbxScene->GetGlobalSettings().GetSystemUnit();
	if (SceneSystemUnit.GetScaleFactor() != 1.0)
	{
		//The unit in this example is centimeter.
		FbxSystemUnit::cm.ConvertScene(pFbxScene);
	}

	// ----------------------------- Converting to Mesh
	// Converting patch, NURBS, mesh into Triangle MESH - 
	//	   i.e. AttributeType will change to eMesh
	FbxGeometryConverter lGeomConverter(g_pFbxSdkManager);
	lGeomConverter.Triangulate(pFbxScene, /*replace*/true);

	// ----------------------------- Getting Mesh
	FbxNode* pFbxRootNode = pFbxScene->GetRootNode();
	uint16_t index = 0;

	if (pFbxRootNode)
	{
		// TODO: 
		//		Temproray set child count to 1, need to add support for per child meshes
		//int childCount = pFbxRootNode->GetChildCount();
		int childCount = pFbxRootNode->GetChildCount() > 0 ? 1 : 0;

		for (uint16_t i = 0; i < childCount; i++)
		{
			FbxNode* pFbxChildNode = pFbxRootNode->GetChild(i);

			if (pFbxChildNode->GetNodeAttribute() == NULL)
				continue;

			FbxNodeAttribute::EType AttributeType = pFbxChildNode->GetNodeAttribute()->GetAttributeType();

			if (AttributeType != FbxNodeAttribute::eMesh)
				continue;

			FbxMesh* pMesh = (FbxMesh*)pFbxChildNode->GetNodeAttribute();

			FbxVector4* pVertices = pMesh->GetControlPoints();

			for (uint16_t j = 0; j < pMesh->GetPolygonCount(); j++)
			{
				uint16_t iNumVertices = pMesh->GetPolygonSize(j);
				assert(iNumVertices == 3);

				for (int k = 0; k < iNumVertices; k++) {
					int iControlPointIndex = pMesh->GetPolygonVertex(j, k);

					VertexPosColor vertex;
					vertex.Position.x = (float)pVertices[iControlPointIndex].mData[0];
					vertex.Position.y = (float)pVertices[iControlPointIndex].mData[1];
					vertex.Position.z = (float)pVertices[iControlPointIndex].mData[2];

					vertex.Color.x = rand() / float(RAND_MAX);
					vertex.Color.y = rand() / float(RAND_MAX);
					vertex.Color.z = rand() / float(RAND_MAX);

					std::set<viPair>::iterator it = setVertInd.find(std::make_pair(vertex, 0/*this value doesn't matter*/));
					if (it != setVertInd.end()) pOutIndices->push_back(it->second);
					else
					{
						setVertInd.insert(std::make_pair(vertex, index));
						pOutIndices->push_back(index++);
					}

				}
			}
		}

		// Notice that the vertices in the set are not sorted by the index
		// so you'll have to rearrange them like this:
		pOutVertices->resize(setVertInd.size());
		for (std::set<viPair>::iterator it = setVertInd.begin(); it != setVertInd.end(); it++) {
			VertexPosColor v = it->first;
			uint16_t i = it->second;

			pOutVertices->at(i) = v;
		}
	}
	return true;
}

#else

// Loads the FBX file and retrives vertices and indices - loading is too slow.
// Loading is to slow because of linear search in pOutVertices array.
// Complexity is INSANE as FIND time increases with newly added vetices. 
bool LoadFBX(const char * fbxFilePath, std::vector<VertexPosColor> * pOutVertices, std::vector<uint16_t> * pOutIndices)
{
	if (g_pFbxSdkManager == nullptr)
	{
		g_pFbxSdkManager = FbxManager::Create();

		FbxIOSettings* pIOsettings = FbxIOSettings::Create(g_pFbxSdkManager, IOSROOT);
		g_pFbxSdkManager->SetIOSettings(pIOsettings);
	}

	FbxImporter* pImporter = FbxImporter::Create(g_pFbxSdkManager, "");
	FbxScene* pFbxScene = FbxScene::Create(g_pFbxSdkManager, "");

	bool bSuccess = pImporter->Initialize(fbxFilePath, -1, g_pFbxSdkManager->GetIOSettings());
	if (!bSuccess) return false;

	bSuccess = pImporter->Import(pFbxScene);
	if (!bSuccess) return false;

	pImporter->Destroy();
	
	// ----------------------------- Convert Axis
	// Convert Axis System to what is used in this example, if needed
	FbxAxisSystem SceneAxisSystem = pFbxScene->GetGlobalSettings().GetAxisSystem();
	FbxAxisSystem OurAxisSystem(FbxAxisSystem::eYAxis, FbxAxisSystem::eParityOdd, FbxAxisSystem::eRightHanded);
	if (SceneAxisSystem != OurAxisSystem)
	{
		OurAxisSystem.ConvertScene(pFbxScene);
	}

	// Convert Unit System to what is used in this example, if needed
	FbxSystemUnit SceneSystemUnit = pFbxScene->GetGlobalSettings().GetSystemUnit();
	if (SceneSystemUnit.GetScaleFactor() != 1.0)
	{
		//The unit in this example is centimeter.
		FbxSystemUnit::cm.ConvertScene(pFbxScene);
	}

	// ----------------------------- Converting to Mesh
	// Converting patch, NURBS, mesh into Triangle MESH - 
	//	   i.e. AttributeType will change to eMesh
	FbxGeometryConverter lGeomConverter(g_pFbxSdkManager);
	lGeomConverter.Triangulate(pFbxScene, /*replace*/true);

	// ----------------------------- Getting Mesh
	FbxNode* pFbxRootNode = pFbxScene->GetRootNode();

	if (pFbxRootNode)
	{
		// TODO: 
		//		Temproray set child count to 1, need to add support for per child meshes
		//int childCount = pFbxRootNode->GetChildCount();
		int childCount = pFbxRootNode->GetChildCount() > 0 ? 1 : 0;

		for (uint16_t i = 0; i < childCount; i++)
		{
			FbxNode* pFbxChildNode = pFbxRootNode->GetChild(i);

			if (pFbxChildNode->GetNodeAttribute() == NULL)
				continue;

			FbxNodeAttribute::EType AttributeType = pFbxChildNode->GetNodeAttribute()->GetAttributeType();

			if (AttributeType != FbxNodeAttribute::eMesh)
				continue;

			FbxMesh* pMesh = (FbxMesh*)pFbxChildNode->GetNodeAttribute();

			FbxVector4* pVertices = pMesh->GetControlPoints();

			for (uint16_t j = 0; j < pMesh->GetPolygonCount(); j++)
			{
				uint16_t iNumVertices = pMesh->GetPolygonSize(j);
				assert(iNumVertices == 3);

				for (int k = 0; k < iNumVertices; k++) {
					int iControlPointIndex = pMesh->GetPolygonVertex(j, k);

					VertexPosColor vertex;
					vertex.Position.x = (float)pVertices[iControlPointIndex].mData[0];
					vertex.Position.y = (float)pVertices[iControlPointIndex].mData[1];
					vertex.Position.z = (float)pVertices[iControlPointIndex].mData[2];

					vertex.Color.x = rand() / float(RAND_MAX);
					vertex.Color.y = rand() / float(RAND_MAX);
					vertex.Color.z = rand() / float(RAND_MAX);

#ifdef INDEXED_DRAW_SUPPORTED
					std::vector<VertexPosColor>::iterator itFind = std::find(pOutVertices->begin(), pOutVertices->end(), vertex);
					if (itFind == pOutVertices->end()) 
					{
						uint16_t index = pOutVertices->size();
						pOutVertices->push_back(vertex);

						pOutIndices->push_back(index);
					}
					else 
					{
						uint16_t indexFromIt = std::distance(pOutVertices->begin(), itFind);
						pOutIndices->push_back(indexFromIt);
					}
#else
					pOutVertices->push_back(vertex);
					pOutIndices->push_back(0);
#endif
				}
			}

		}
	}
	return true;
}
#endif 