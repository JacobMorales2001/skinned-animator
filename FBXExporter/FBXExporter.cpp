#include <windows.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <directxmath.h>


#include <fbxsdk.h>
#include <vector>
#include <iostream>
#include <fstream>

namespace MFBXExporter
{
	using namespace DirectX;

	struct MoralesVertex
	{
		XMFLOAT3 Pos;
		XMFLOAT3 Normal;
		XMFLOAT2 Tex;
	};

	struct MoralesMesh
	{
		std::vector<MoralesVertex> vertexList;
		std::vector<int> indicesList;
	};

	void ProcessFbxMesh(FbxNode* Node);
	void SaveMesh(const char* meshFileName, MoralesMesh& mesh);
	std::string ReplaceFBXExtension(std::string fileName);

	MoralesMesh simpleMesh;
	int numIndices = 0;

	int main(int argc, char** argv)
	{
		// set up output console
		AllocConsole();
		freopen("CONOUT$", "w", stdout);
		freopen("CONOUT$", "w", stderr);

		if (argc != 2)
		{
			std::cout << "Invalid arguments.\n";
			std::cout << "First argument must be source file with extension EX: \"Assets/MyFBXFile.fbx\".\n";
			std::cout << "exiting.\n";
			std::cin.get();
			exit(-1);
		}

		std::cout << "File to read: " << argv[1] << '\n';

		std::string SourceFileLocation = argv[1];

		std::string fbx = ".fbx";
		std::string mesh = ".mbm"; // Morales Binary Mesh

		// Initialize the SDK manager. This object handles memory management.
		FbxManager* lSdkManager = FbxManager::Create();

		// Create the IO settings object.
		FbxIOSettings* ios = FbxIOSettings::Create(lSdkManager, IOSROOT);
		lSdkManager->SetIOSettings(ios);

		// Create an importer using the SDK manager.
		FbxImporter* lImporter = FbxImporter::Create(lSdkManager, "");

		// Use the first argument as the filename for the importer.
		if (!lImporter->Initialize(SourceFileLocation.c_str(), -1, lSdkManager->GetIOSettings())) {
			printf("Call to FbxImporter::Initialize() failed.\n");
			printf("Error returned: %s\n\n", lImporter->GetStatus().GetErrorString());
			std::cin.get();
			exit(-1);
		}

		// Create a new scene so that it can be populated by the imported file.
		FbxScene* lScene = FbxScene::Create(lSdkManager, "myScene");

		// Import the contents of the file into the scene.
		lImporter->Import(lScene);

		// The file is imported, so get rid of the importer.
		lImporter->Destroy();

		ProcessFbxMesh(lScene->GetRootNode());

		std::string newFileLocation = ReplaceFBXExtension(SourceFileLocation);

		SaveMesh(newFileLocation.c_str(), simpleMesh);

		// Destroy the SDK manager and all the other objects it was handling.
		lSdkManager->Destroy();

		std::cout << "\n\nFile exported successfully . . .\n";
		std::cout << "File saved as: " << newFileLocation << '\n';
		std::cin.get();

		// return 0 for success.
		return 0;
	}

	void ProcessFbxMesh(FbxNode* Node)
	{


		//FBX Mesh stuff
		int childrenCount = Node->GetChildCount();

		std::cout << "\nName:" << Node->GetName();

		for (int i = 0; i < childrenCount; i++)
		{
			FbxNode* childNode = Node->GetChild(i);
			FbxMesh* mesh = childNode->GetMesh();

			if (mesh != NULL)
			{
				std::cout << "\nMesh:" << childNode->GetName();

				// Get index count from mesh
				int numVertices = mesh->GetControlPointsCount();
				std::cout << "\nVertex Count:" << numVertices;

				// Resize the vertex vector to size of this mesh
				simpleMesh.vertexList.resize(numVertices);

				//================= Process Vertices ===============
				for (int j = 0; j < numVertices; j++)
				{
					FbxVector4 vert = mesh->GetControlPointAt(j);
					simpleMesh.vertexList[j].Pos.x = static_cast<float>(vert.mData[0]);
					simpleMesh.vertexList[j].Pos.y = static_cast<float>(vert.mData[1]);
					simpleMesh.vertexList[j].Pos.z = static_cast<float>(vert.mData[2]);
					// Generate random normal for first attempt at getting to render
					//simpleMesh.vertexList[j].Normal = RAND_NORMAL;
				}

				numIndices = mesh->GetPolygonVertexCount();
				std::cout << "\nIndice Count:" << numIndices;

				// No need to allocate int array, FBX does for us
				int* indices = mesh->GetPolygonVertices();

				// Fill indiceList
				simpleMesh.indicesList.resize(numIndices);
				memcpy(simpleMesh.indicesList.data(), indices, numIndices * sizeof(int));

				// Get the Normals array from the mesh
				FbxArray<FbxVector4> normalsVec;
				mesh->GetPolygonVertexNormals(normalsVec);
				std::cout << "\nNormalVec Count:" << normalsVec.Size();

				// Declare a new array for the second vertex array
				// Note the size is numIndices not numVertices
				std::vector<MoralesVertex> vertexListExpanded;
				vertexListExpanded.resize(numIndices);

				int CurrentUV = 0;

				//get all UV set names
				FbxStringList lUVSetNameList;
				mesh->GetUVSetNames(lUVSetNameList);

				//iterating over all uv sets
				for (int lUVSetIndex = 0; lUVSetIndex < lUVSetNameList.GetCount(); lUVSetIndex++)
				{
					//get lUVSetIndex-th uv set
					const char* lUVSetName = lUVSetNameList.GetStringAt(lUVSetIndex);
					const FbxGeometryElementUV* lUVElement = mesh->GetElementUV(lUVSetName);

					if (!lUVElement)
						continue;

					// only support mapping mode eByPolygonVertex and eByControlPoint
					if (lUVElement->GetMappingMode() != FbxGeometryElement::eByPolygonVertex &&
						lUVElement->GetMappingMode() != FbxGeometryElement::eByControlPoint)
						return;

					//index array, where holds the index referenced to the uv data
					const bool lUseIndex = lUVElement->GetReferenceMode() != FbxGeometryElement::eDirect;
					const int lIndexCount = (lUseIndex) ? lUVElement->GetIndexArray().GetCount() : 0;

					//iterating through the data by polygon
					const int lPolyCount = mesh->GetPolygonCount();

					if (lUVElement->GetMappingMode() == FbxGeometryElement::eByControlPoint)
					{
						for (int lPolyIndex = 0; lPolyIndex < lPolyCount; ++lPolyIndex)
						{
							// build the max index array that we need to pass into MakePoly
							const int lPolySize = mesh->GetPolygonSize(lPolyIndex);
							for (int lVertIndex = 0; lVertIndex < lPolySize; ++lVertIndex)
							{
								FbxVector2 lUVValue;

								//get the index of the current vertex in control points array
								int lPolyVertIndex = mesh->GetPolygonVertex(lPolyIndex, lVertIndex);

								//the UV index depends on the reference mode
								int lUVIndex = lUseIndex ? lUVElement->GetIndexArray().GetAt(lPolyVertIndex) : lPolyVertIndex;

								lUVValue = lUVElement->GetDirectArray().GetAt(lUVIndex);

								//User TODO:
								//Print out the value of UV(lUVValue) or log it to a file
							}
						}
					}
					else if (lUVElement->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
					{
						int lPolyIndexCounter = 0;
						for (int lPolyIndex = 0; lPolyIndex < lPolyCount; ++lPolyIndex)
						{
							// build the max index array that we need to pass into MakePoly
							const int lPolySize = mesh->GetPolygonSize(lPolyIndex);
							for (int lVertIndex = 0; lVertIndex < lPolySize; ++lVertIndex)
							{
								if (lPolyIndexCounter < lIndexCount)
								{
									FbxVector2 lUVValue;

									//the UV index depends on the reference mode
									int lUVIndex = lUseIndex ? lUVElement->GetIndexArray().GetAt(lPolyIndexCounter) : lPolyIndexCounter;

									lUVValue = lUVElement->GetDirectArray().GetAt(lUVIndex);

									//User TODO:
									//Print out the value of UV(lUVValue) or log it to a file
									//lUVValue.mData[]
									//std::cout << "\n" << CurrentUV << " UV Info : X = " << lUVValue.mData[0] << " Y = " << lUVValue.mData[1] << "\n";
									vertexListExpanded[CurrentUV].Tex.x = lUVValue.mData[0];
									vertexListExpanded[CurrentUV].Tex.y = lUVValue.mData[1];

									CurrentUV++;

									lPolyIndexCounter++;
								}
							}
						}
					}
				}

				// align (expand) vertex array and set the normals
				for (int j = 0; j < numIndices; j++)
				{
					vertexListExpanded[j].Pos = simpleMesh.vertexList[simpleMesh.indicesList[j]].Pos;
					vertexListExpanded[j].Normal.x = normalsVec.GetAt(j)[0];
					vertexListExpanded[j].Normal.y = normalsVec.GetAt(j)[1];
					vertexListExpanded[j].Normal.z = normalsVec.GetAt(j)[2];
				}

				// make new indices to match the new vertex(2) array
				std::vector<int> indicesList;
				indicesList.resize(numIndices);
				for (int j = 0; j < numIndices; j++)
				{
					indicesList[j] = j;
					//std::cout << "\n" << indicesList[j];
				}

				if (vertexListExpanded.size() > simpleMesh.vertexList.size())
				{
					// compactify
					int expandedSize = vertexListExpanded.size();
					std::vector<MoralesVertex> compactedVertexList;
					std::vector<int> compactedIndexList;
					compactedIndexList.resize(numIndices);
					bool thereIsACopy = false;
					for (int j = 0; j < expandedSize; j++)
					{
						thereIsACopy = false;
						int foundAt = 0;
						for (int k = 0; k < compactedVertexList.size(); k++)
						{
							if
								(
									(vertexListExpanded[j].Normal.x == compactedVertexList[k].Normal.x) &&
									(vertexListExpanded[j].Normal.y == compactedVertexList[k].Normal.y) &&
									(vertexListExpanded[j].Normal.z == compactedVertexList[k].Normal.z) &&
									(vertexListExpanded[j].Pos.x == compactedVertexList[k].Pos.x) &&
									(vertexListExpanded[j].Pos.y == compactedVertexList[k].Pos.y) &&
									(vertexListExpanded[j].Pos.z == compactedVertexList[k].Pos.z) &&
									(vertexListExpanded[j].Tex.x == compactedVertexList[k].Tex.x) &&
									(vertexListExpanded[j].Tex.y == compactedVertexList[k].Tex.y)
									)
							{
								compactedIndexList[j] = k;
								thereIsACopy = true;
								break;
							}
						}
						if (!thereIsACopy)
						{
							compactedVertexList.push_back(vertexListExpanded[j]);
							compactedIndexList[j] = compactedVertexList.size() - 1;
						}
					}



					// copy working data to the global SimpleMesh
					simpleMesh.indicesList.assign(compactedIndexList.begin(), compactedIndexList.end());
					simpleMesh.vertexList.assign(compactedVertexList.begin(), compactedVertexList.end());

					// print out some stats
					std::cout << "\nindex count BEFORE/AFTER compaction " << numIndices;
					std::cout << "\nvertex count ORIGINAL (FBX source): " << numVertices;
					std::cout << "\nvertex count AFTER expansion: " << numIndices;
					std::cout << "\nvertex count AFTER compaction: " << simpleMesh.vertexList.size();
					std::cout << "\nSize reduction: " << ((numVertices - simpleMesh.vertexList.size()) / (float)numVertices) * 100.00f << "%";
					std::cout << "\nor " << (simpleMesh.vertexList.size() / (float)numVertices) << " of the expanded size";
				}
				else
				{
					// copy working data to the global SimpleMesh
					simpleMesh.indicesList.assign(indicesList.begin(), indicesList.end());
					simpleMesh.vertexList.assign(vertexListExpanded.begin(), vertexListExpanded.end());
				}

				// Print out the mesh's texture file
				int materialCount = childNode->GetSrcObjectCount<FbxSurfaceMaterial>();

				for (int index = 0; index < materialCount; index++)
				{
					FbxSurfaceMaterial* material = (FbxSurfaceMaterial*)childNode->GetSrcObject<FbxSurfaceMaterial>(index);
					
					if (material != NULL)
					{
						std::cout << "\nmaterial: " << material->GetName() << std::endl;
						// This only gets the material of type sDiffuse, you probably need to traverse all Standard Material Property by its name to get all possible textures.
						FbxProperty prop = material->FindProperty(FbxSurfaceMaterial::sDiffuse);
						//material->FindProperty(FbxSurfaceMaterial::);

						// Check if it's layeredtextures
						int layeredTextureCount = prop.GetSrcObjectCount<FbxLayeredTexture>();

						if (layeredTextureCount > 0)
						{
							for (int j = 0; j < layeredTextureCount; j++)
							{
								FbxLayeredTexture* layered_texture = FbxCast<FbxLayeredTexture>(prop.GetSrcObject<FbxLayeredTexture>(j));
								int lcount = layered_texture->GetSrcObjectCount<FbxTexture>();

								for (int k = 0; k < lcount; k++)
								{
									FbxFileTexture* texture = FbxCast<FbxFileTexture>(layered_texture->GetSrcObject<FbxTexture>(k));
									// Then, you can get all the properties of the texture, include its name
									const char* textureName = texture->GetFileName();
									std::cout << textureName;
								}
							}
						}
						else
						{
							// Directly get textures
							int textureCount = prop.GetSrcObjectCount<FbxTexture>();
							for (int j = 0; j < textureCount; j++)
							{
								FbxFileTexture* texture = FbxCast<FbxFileTexture>(prop.GetSrcObject<FbxTexture>(j));
								// Then, you can get all the properties of the texture, include its name
								const char* textureName = texture->GetFileName();
								std::cout << textureName;

								FbxProperty p = texture->RootProperty.Find("Filename");
								std::cout << p.Get<FbxString>() << std::endl;

							}
						}
					}
				}

			}
		}
	}

	void SaveMesh(const char* meshFileName, MoralesMesh& mesh)
	{
		std::ofstream file(meshFileName, std::ios::trunc | std::ios::binary | std::ios::out);

		assert(file.is_open());

		uint32_t index_count = (uint32_t)mesh.indicesList.size();
		uint32_t vert_count = (uint32_t)mesh.vertexList.size();

		file.write((const char*)&index_count, sizeof(uint32_t));
		file.write((const char*)mesh.indicesList.data(), sizeof(uint32_t) * mesh.indicesList.size());
		file.write((const char*)&vert_count, sizeof(uint32_t));
		file.write((const char*)mesh.vertexList.data(), sizeof(MoralesVertex) * mesh.vertexList.size());
		file.close();
	}

	std::string ReplaceFBXExtension(std::string fileName)
	{
		fileName.replace(fileName.end() - 3, fileName.end(), "mbm");

		return fileName;
	}


}

int main(int argc, char** argv)
{
	return MFBXExporter::main(argc, argv);
}