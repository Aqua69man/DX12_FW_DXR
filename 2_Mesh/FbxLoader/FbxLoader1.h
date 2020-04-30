#pragma once

#include <fbxsdk.h>
#include <vector>
#include <assert.h>
#include <unordered_map>
#include <set>


#include <DirectXMath.h>
using namespace DirectX;

#define INDEXED_DRAW_SUPPORTED

// Vertex data for a colored cube.
struct VertexPosColor
{
	XMFLOAT3 Position;
	XMFLOAT3 Color;

	bool operator==(const VertexPosColor & vertex) {
		return ( Position.x == vertex.Position.x &&
				 Position.y == vertex.Position.y &&
				 Position.z == vertex.Position.z
				);
	}
};

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

	if (pFbxRootNode)
	{
		for (int i = 0; i < pFbxRootNode->GetChildCount(); i++)
		{
			FbxNode* pFbxChildNode = pFbxRootNode->GetChild(i);

			if (pFbxChildNode->GetNodeAttribute() == NULL)
				continue;

			FbxNodeAttribute::EType AttributeType = pFbxChildNode->GetNodeAttribute()->GetAttributeType();

			if (AttributeType != FbxNodeAttribute::eMesh)
				continue;

			FbxMesh* pMesh = (FbxMesh*)pFbxChildNode->GetNodeAttribute();

			FbxVector4* pVertices = pMesh->GetControlPoints();

			for (int j = 0; j < pMesh->GetPolygonCount(); j++)
			{
				int iNumVertices = pMesh->GetPolygonSize(j);
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

bool LoadFBX_new(const char * fbxFilePath, std::vector<VertexPosColor> * pOutVertices, std::vector<uint16_t> * pOutIndices)
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

	// ----------------------------- Retrive data
	FbxNode* pFbxRootNode = pFbxScene->GetRootNode();

	if (pFbxRootNode)
	{
		for (int i = 0; i < pFbxRootNode->GetChildCount(); i++)
		{
			FbxNode* pFbxChildNode = pFbxRootNode->GetChild(i);

			if (pFbxChildNode->GetNodeAttribute() == NULL)
				continue;

			FbxNodeAttribute::EType AttributeType = pFbxChildNode->GetNodeAttribute()->GetAttributeType();

			if (AttributeType != FbxNodeAttribute::eMesh)
				continue;

			FbxMesh* pMesh = (FbxMesh*)pFbxChildNode->GetNodeAttribute();

			FbxVector4* pVertices = pMesh->GetControlPoints();

			for (int j = 0; j < pMesh->GetPolygonCount(); j++)
			{
				int iNumVertices = pMesh->GetPolygonSize(j);
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

					UINT ii = j * iNumVertices + k;

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

				}
			}

		}
	}
	return true;
}