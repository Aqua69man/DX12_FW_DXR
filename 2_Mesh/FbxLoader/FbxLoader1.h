#pragma once

#include <fbxsdk.h>
#include <assert.h>

#include <vector>
#include <set>
//#include <cmath>

#include <DirectXMath.h>
using namespace DirectX;

#define USE_OPTIMIZAED_INDICES

// Vertex data for a colored cube.
struct VertexPosColor
{
	XMFLOAT3 Position;
	XMFLOAT3 Color;
	XMFLOAT3 Normal;
	XMFLOAT2 UV;
};
typedef std::pair<VertexPosColor, uint16_t> viPair;

// Class comparator for vertices in the set
struct CmpClass 
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

			bool hasNormal = pMesh->GetElementNormalCount() > 0;
			bool hasUV = pMesh->GetElementUVCount() > 0;
			bool lUnmappedUV;
			FbxStringList lUVNames;
			pMesh->GetUVSetNames(lUVNames);
			const char * lUVName = NULL;
			if (hasUV && lUVNames.GetCount())
			{
				lUVName = lUVNames[0];
			}

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

					if (hasNormal) {
						FbxVector4 lCurrentNormal;
						pMesh->GetPolygonVertexNormal(j, k, lCurrentNormal);
						vertex.Normal.x = static_cast<float>(lCurrentNormal[0]);
						vertex.Normal.y = static_cast<float>(lCurrentNormal[1]);
						vertex.Normal.z = static_cast<float>(lCurrentNormal[2]);
					}

					if (hasUV)
					{
						FbxVector2 lCurrentUV;
						pMesh->GetPolygonVertexUV(j, k, lUVName, lCurrentUV, lUnmappedUV);
						vertex.UV.x = static_cast<float>(lCurrentUV[0]);
						vertex.UV.y = static_cast<float>(lCurrentUV[1]);
					}

#ifdef USE_OPTIMIZAED_INDICES
					std::set<viPair>::iterator it = setVertInd.find(std::make_pair(vertex, 0/*this value doesn't matter*/));
					if (it != setVertInd.end()) pOutIndices->push_back(it->second);
					else
					{
						setVertInd.insert(std::make_pair(vertex, index));
						pOutIndices->push_back(index++);
					}
#else 
					pOutVertices->push_back(vertex);
					pOutIndices->push_back(index++);
#endif
				}
			}
		}

#ifdef USE_OPTIMIZAED_INDICES
		// Notice that the vertices in the set are not sorted by the index
		// so you'll have to rearrange them like this:
		pOutVertices->resize(setVertInd.size());
		for (std::set<viPair>::iterator it = setVertInd.begin(); it != setVertInd.end(); it++) {
			VertexPosColor v = it->first;
			uint16_t i = it->second;

			pOutVertices->at(i) = v;
		}
#endif 

	}
	return true;
}