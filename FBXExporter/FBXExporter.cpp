#include <windows.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <directxmath.h>

#include <fbxsdk.h>
#include <vector>
#include <iostream>
#include <iomanip>
#include <fstream>

namespace MFBXExporter
{
	constexpr int maxPathLength = 260;
	constexpr int MAX_INFLUENCES = 4;
	using namespace DirectX;

	using ulong = unsigned long long;

	struct MoralesVertex
	{
		XMFLOAT4 Pos;
		XMFLOAT4 Normal;
		XMFLOAT2 Tex;
		XMINT4 Joints;
		struct Double4 { double x, y, z, w; } Weights;
	};

	struct MoralesFbxJoint
	{
		FbxNode* node;
		int parentIndex;
	};

	struct MoralesJoint
	{
		float globalTransform[16];
		int parentIndex;
	};

	using MoralesPose = std::vector<MoralesJoint>;

	struct MoralesKeyframe
	{
		double keytime;
		MoralesPose poseData;
	};

	struct MoralesAnimation
	{
		double duration;
		std::vector<MoralesKeyframe> keyframes;
	};

	struct MoralesMaterial
	{
		enum ComponentType {EMISSIVE = 0, DIFFUSE, SPECULAR, SHININESS, COUNT};

		struct Component
		{
			float value[3] = { 0.0f, 0.0f, 0.0f };
			float factor = 0.0f;
			int64_t input = -1;
		};

		Component& operator[](int i) { return components[i]; }

		const Component& operator[](int i) const { return components[i]; }

	private:
		Component components[COUNT];
	};

	struct MoralesMesh
	{
		std::vector<MoralesVertex> vertexList;
		std::vector<int> indicesList;
		std::vector<MoralesMaterial> materialList;
		std::vector<std::string> materialPaths;
		MoralesPose bindPose;
		MoralesAnimation animation; //TODO: maybe add support for multiple animation loading?
	};

	struct MoralesInfluence
	{
		int joint; 
		float weight;
	};

	struct MoralesInfluenceSet
	{
		MoralesInfluence infs[4];
	};


	std::vector<MoralesInfluenceSet> controlPointInfluences;

	//using MoralesInfluenceSet = std::array<MoralesInfluence, 4>;



	void ProcessFbxMesh(FbxNode* Node);
	void ProcessFbxMaterials(FbxScene* Scene);
	void ProcessFbxAnimation(FbxScene* Scene);
	void SaveMesh(const char* meshFileName, MoralesMesh& mesh);
	std::string ReplaceFBXExtension(std::string fileName);
	bool AreEqual(float a, float b);
	void ConvertFbxAMatrixToFloat16(float* m, const FbxAMatrix& mat);
	std::string OpenFileName(const wchar_t* filter, HWND owner);
	void AddAndKeepArraySorted(MoralesInfluenceSet& mis, MoralesInfluence& mi);
	void SortMIS(MoralesInfluenceSet& mis);

	MoralesMesh moralesMesh;
	int numIndices = 0;
	int numControlPoints = 0;

	int main(int argc, char** argv)
	{
		// set up output console
		AllocConsole();
		freopen("CONOUT$", "w", stdout);
		freopen("CONOUT$", "w", stderr);

		//if (argc != 2)
		//{
		//	std::cout << "Invalid arguments.\n";
		//	std::cout << "First argument must be source file with extension EX: \"Assets/MyFBXFile.fbx\".\n";
		//	std::cout << "exiting.\n";
		//	std::cin.get();
		//	exit(-1);
		//}

		std::string fn = OpenFileName(L"Autodesk .fbx Files (*.fbx)\0*.fbx*\0", NULL);

		std::cout << "File to read: " << fn << '\n';

		std::string SourceFileLocation = fn;

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

		// Process the mesh and the materials
		ProcessFbxAnimation(lScene);

		ProcessFbxMesh(lScene->GetRootNode());

		ProcessFbxMaterials(lScene);



		std::string newFileLocation = ReplaceFBXExtension(SourceFileLocation);

		SaveMesh(newFileLocation.c_str(), moralesMesh);

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
				numControlPoints = numVertices;
				std::cout << "\nVertex Count:" << numVertices;

				// Resize the vertex vector to size of this mesh
				moralesMesh.vertexList.resize(numVertices);

				//================= Process Vertices ===============
				for (int j = 0; j < numVertices; j++)
				{
					FbxVector4 vert = mesh->GetControlPointAt(j);
					moralesMesh.vertexList[j].Pos.x = static_cast<float>(vert.mData[0]);
					moralesMesh.vertexList[j].Pos.y = static_cast<float>(vert.mData[1]);
					moralesMesh.vertexList[j].Pos.z = static_cast<float>(vert.mData[2]);
					moralesMesh.vertexList[j].Pos.w = static_cast<float>(vert.mData[3]);



					// Generate random normal for first attempt at getting to render
					//simpleMesh.vertexList[j].Normal = RAND_NORMAL;
				}

				numIndices = mesh->GetPolygonVertexCount();
				std::cout << "\nIndice Count:" << numIndices;

				// No need to allocate int array, FBX does for us
				int* indices = mesh->GetPolygonVertices();

				// Fill indiceList
				moralesMesh.indicesList.resize(numIndices);
				memcpy(moralesMesh.indicesList.data(), indices, numIndices * sizeof(int));

				// Get the Normals array from the mesh
				FbxArray<FbxVector4> normalsVec;
				mesh->GetPolygonVertexNormals(normalsVec);
				std::cout << "\nNormalVec Count:" << normalsVec.Size() << "\n\n";

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

								vertexListExpanded[CurrentUV].Tex.x = lUVValue.mData[0];
								vertexListExpanded[CurrentUV].Tex.y = lUVValue.mData[1];

								CurrentUV++;

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

				// set all of the control points
				for (int i = 0; i < numControlPoints; i++)
				{
					moralesMesh.vertexList[i].Joints.x = controlPointInfluences[i].infs[0].joint;
					moralesMesh.vertexList[i].Joints.y = controlPointInfluences[i].infs[1].joint;
					moralesMesh.vertexList[i].Joints.z = controlPointInfluences[i].infs[2].joint;
					moralesMesh.vertexList[i].Joints.w = controlPointInfluences[i].infs[3].joint;

					moralesMesh.vertexList[i].Weights.x = controlPointInfluences[i].infs[0].weight;
					moralesMesh.vertexList[i].Weights.y = controlPointInfluences[i].infs[1].weight;
					moralesMesh.vertexList[i].Weights.z = controlPointInfluences[i].infs[2].weight;
					moralesMesh.vertexList[i].Weights.w = controlPointInfluences[i].infs[3].weight;

					double sum = moralesMesh.vertexList[i].Weights.x + moralesMesh.vertexList[i].Weights.y +
						moralesMesh.vertexList[i].Weights.z + moralesMesh.vertexList[i].Weights.w;

					moralesMesh.vertexList[i].Weights.x /= sum;
					moralesMesh.vertexList[i].Weights.y /= sum;
					moralesMesh.vertexList[i].Weights.z /= sum;
					moralesMesh.vertexList[i].Weights.w /= sum;
				}
				std::cout << "Mapped control influences to vertices\n";
				
				// align (expand) vertex array and set the normals
				for (int j = 0; j < numIndices; j++)
				{
					vertexListExpanded[j].Pos = moralesMesh.vertexList[moralesMesh.indicesList[j]].Pos;
					vertexListExpanded[j].Normal.x = normalsVec.GetAt(j)[0];
					vertexListExpanded[j].Normal.y = normalsVec.GetAt(j)[1];
					vertexListExpanded[j].Normal.z = normalsVec.GetAt(j)[2];
					vertexListExpanded[j].Normal.w = normalsVec.GetAt(j)[3];
					vertexListExpanded[j].Joints = moralesMesh.vertexList[moralesMesh.indicesList[j]].Joints;
					vertexListExpanded[j].Weights = moralesMesh.vertexList[moralesMesh.indicesList[j]].Weights;
				}

				// make new indices to match the new vertex(2) array
				std::vector<int> indicesList;
				indicesList.resize(numIndices);
				for (int j = 0; j < numIndices; j++)
				{
					indicesList[j] = j;
					//std::cout << "\n" << indicesList[j];
				}

				if (vertexListExpanded.size() > moralesMesh.vertexList.size())
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
							// if the two verts are pretty close to being equal
							if ((AreEqual(vertexListExpanded[j].Normal.x, compactedVertexList[k].Normal.x)) &&
								(AreEqual(vertexListExpanded[j].Normal.y, compactedVertexList[k].Normal.y)) &&
								(AreEqual(vertexListExpanded[j].Normal.z, compactedVertexList[k].Normal.z)) &&
								(AreEqual(vertexListExpanded[j].Normal.w, compactedVertexList[k].Normal.w)) &&
								(AreEqual(vertexListExpanded[j].Pos.x, compactedVertexList[k].Pos.x)) &&
								(AreEqual(vertexListExpanded[j].Pos.y, compactedVertexList[k].Pos.y)) &&
								(AreEqual(vertexListExpanded[j].Pos.z, compactedVertexList[k].Pos.z)) &&
								(AreEqual(vertexListExpanded[j].Pos.w, compactedVertexList[k].Pos.w)))
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
					moralesMesh.indicesList.assign(compactedIndexList.begin(), compactedIndexList.end());
					moralesMesh.vertexList.assign(compactedVertexList.begin(), compactedVertexList.end());

					// print out some stats
					std::cout << "\nindex count BEFORE/AFTER compaction " << numIndices;
					std::cout << "\nvertex count ORIGINAL (FBX source): " << numVertices;
					std::cout << "\nvertex count AFTER expansion: " << numIndices;
					std::cout << "\nvertex count AFTER compaction: " << moralesMesh.vertexList.size();
					std::cout << "\nSize reduction: " << ((numVertices - moralesMesh.vertexList.size()) / (float)numVertices) * 100.00f << "%";
					std::cout << "\nor " << (moralesMesh.vertexList.size() / (float)numVertices) << " of the expanded size\n\n";
				}
				else
				{
					// copy working data to the global SimpleMesh
					moralesMesh.indicesList.assign(indicesList.begin(), indicesList.end());
					moralesMesh.vertexList.assign(vertexListExpanded.begin(), vertexListExpanded.end());
				}

				// Print out the mesh's texture file
				int materialCount = childNode->GetSrcObjectCount<FbxSurfaceMaterial>();

				for (int index = 0; index < materialCount; index++)
				{
					FbxSurfaceMaterial* material = (FbxSurfaceMaterial*)childNode->GetSrcObject<FbxSurfaceMaterial>(index);
					
					if (material != NULL)
					{
						//std::cout << "\n OLD********* -- material: " << material->GetName() << std::endl;
						// This only gets the material of type sDiffuse, you probably need to traverse all Standard Material Property by its name to get all possible textures.
						FbxProperty prop = material->FindProperty(FbxSurfaceMaterial::sDiffuse);

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
									//std::cout << " OLD********* -- " << textureName;
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
								//std::cout << " OLD********* -- " << textureName;

								FbxProperty p = texture->RootProperty.Find("Filename");
								//std::cout << " OLD********* -- " << p.Get<FbxString>() << std::endl;

							}
						}
					}
				}

			}
		}
	}

	void ProcessFbxMaterials(FbxScene* Scene)
	{
		int num_mats = Scene->GetMaterialCount();

		for (int i = 0; i < num_mats; i++)
		{
			MoralesMaterial material;
			FbxSurfaceMaterial* mat = Scene->GetMaterial(i);

			if (!mat->Is<FbxSurfaceLambert>()) { continue; }

			FbxSurfaceLambert* lambert = (FbxSurfaceLambert*)mat;
			FbxDouble3 diffuse_color = lambert->Diffuse.Get();
			FbxDouble diffuse_factor = lambert->DiffuseFactor.Get();

			FbxDouble3 emissive_color = lambert->Emissive.Get();
			FbxDouble emissive_factor = lambert->EmissiveFactor.Get();
			
			//FbxDouble3 shininess_color = lambert->.Get();
			//FbxDouble shininess_factor = lambert->EmissiveFactor.Get();

			material[MoralesMaterial::DIFFUSE].value[0] = diffuse_color.mData[0];
			material[MoralesMaterial::DIFFUSE].value[1] = diffuse_color.mData[1];
			material[MoralesMaterial::DIFFUSE].value[2] = diffuse_color.mData[2];
			material[MoralesMaterial::DIFFUSE].factor = diffuse_factor;

			std::cout << "Diffuse Material Color: " << material[MoralesMaterial::DIFFUSE].value[0] << " ";
			std::cout << material[MoralesMaterial::DIFFUSE].value[1] << " "; 
			std::cout << material[MoralesMaterial::DIFFUSE].value[2] << '\n';
			std::cout << "Diffuse Material Factor: " << material[MoralesMaterial::DIFFUSE].factor << '\n';

			if (FbxFileTexture* file_texture_lambert = lambert->Diffuse.GetSrcObject<FbxFileTexture>())
			{
				const char* file_name = file_texture_lambert->GetRelativeFileName(); 
				std::string file_path = file_name; 
				material[MoralesMaterial::DIFFUSE].input = moralesMesh.materialPaths.size(); 
				moralesMesh.materialPaths.push_back(file_path);
				std::cout << "Diffuse Material Filepath: \n\t" << file_path << "\n\n";
			}



			material[MoralesMaterial::EMISSIVE].value[0] = emissive_color.mData[0];
			material[MoralesMaterial::EMISSIVE].value[1] = emissive_color.mData[1];
			material[MoralesMaterial::EMISSIVE].value[2] = emissive_color.mData[2];
			material[MoralesMaterial::EMISSIVE].factor = emissive_factor;

			std::cout << "Emissive Material Color: " << material[MoralesMaterial::EMISSIVE].value[0] << " ";
			std::cout << material[MoralesMaterial::EMISSIVE].value[1] << " ";
			std::cout << material[MoralesMaterial::EMISSIVE].value[2] << '\n';
			std::cout << "Emissive Material Factor: " << material[MoralesMaterial::EMISSIVE].factor << '\n';

			if (FbxFileTexture* file_texture_emissive = lambert->Emissive.GetSrcObject<FbxFileTexture>())
			{
				const char* file_name = file_texture_emissive->GetRelativeFileName();
				std::string file_path = file_name;
				material[MoralesMaterial::EMISSIVE].input = moralesMesh.materialPaths.size();
				moralesMesh.materialPaths.push_back(file_path);
				std::cout << "Emissive Material Filepath: \n\t" << file_path << "\n\n";
			}


			if(mat->Is<FbxSurfacePhong>())
			{
				FbxSurfacePhong* phong = (FbxSurfacePhong*)mat;
				FbxDouble3 phong_color = phong->Specular.Get();
				FbxDouble phong_factor = phong->SpecularFactor.Get();

				material[MoralesMaterial::SPECULAR].value[0] = phong_color.mData[0];
				material[MoralesMaterial::SPECULAR].value[1] = phong_color.mData[1];
				material[MoralesMaterial::SPECULAR].value[2] = phong_color.mData[2];
				material[MoralesMaterial::SPECULAR].factor = phong_factor;

				std::cout << "Specular Material Color: " << material[MoralesMaterial::SPECULAR].value[0] << " ";
				std::cout << material[MoralesMaterial::SPECULAR].value[1] << " ";
				std::cout << material[MoralesMaterial::SPECULAR].value[2] << '\n';
				std::cout << "Specular Material Factor: " << material[MoralesMaterial::SPECULAR].factor << '\n';

				if (FbxFileTexture* file_texture = phong->Specular.GetSrcObject<FbxFileTexture>())
				{
					const char* file_name = file_texture->GetRelativeFileName();
					std::string file_path = file_name;
					material[MoralesMaterial::SPECULAR].input = moralesMesh.materialPaths.size();
					moralesMesh.materialPaths.push_back(file_path);
					std::cout << "Specular Material Filepath: \n\t" << file_path << "\n\n";
				}
			}
			
			moralesMesh.materialList.push_back(material);
		}
	}


	void ProcessFbxAnimation(FbxScene* Scene)
	{
		std::vector<MoralesFbxJoint> joints;
		FbxPose* bindPose = nullptr;
		// Find the first FbxPose that is a bind pose, assume that the first pose is the only pose of interest.
		int poseCount = Scene->GetPoseCount();
		for (int i = 0; i < poseCount; i++)
		{
			FbxPose* pose = Scene->GetPose(i);

			if (pose->IsBindPose())
			{
				bindPose = pose;
				// from the bind pose, find the root of the skeleton
				int nodeCount = pose->GetCount();
				for (int j = 0; j < nodeCount; j++)
				{
					FbxNode* node = pose->GetNode(j);
					FbxSkeleton* skeleton = node->GetSkeleton();
					if (skeleton != NULL && skeleton->IsSkeletonRoot())
					{
						// Starting with the skeleton root, build a dynamic array of FbxNode* paired with parent indices
						joints.push_back({ node, -1 });
						break;
					}
				}
			}
		}

		// build the dynarray
		for (int i = 0; i < joints.size(); ++i)
		{
			FbxNode* currentNode = joints[i].node;
			int childCount = currentNode->GetChildCount();

			for (int j = 0; j < childCount; j++)
			{
				FbxNode* childNode = currentNode->GetChild(j);
				if (childNode->GetNodeAttribute() && childNode->GetNodeAttribute()->GetAttributeType() && childNode->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton)
				{
					joints.push_back({ childNode, i});
				}
			}
		}



		// Now we have an array of FbxNode* with their parents. Evaluate the global transforms
		for (size_t i = 0; i < joints.size(); i++)
		{
			MoralesJoint mjoint;
			mjoint.parentIndex = joints[i].parentIndex;
			FbxAMatrix mat = joints[i].node->EvaluateGlobalTransform();
			mjoint.globalTransform[0] = mat.mData[0][0];
			mjoint.globalTransform[1] = mat.mData[0][1];
			mjoint.globalTransform[2] = mat.mData[0][2];
			mjoint.globalTransform[3] = mat.mData[0][3];

			mjoint.globalTransform[4] = mat.mData[1][0];
			mjoint.globalTransform[5] = mat.mData[1][1];
			mjoint.globalTransform[6] = mat.mData[1][2];
			mjoint.globalTransform[7] = mat.mData[1][3];
		
			mjoint.globalTransform[8]  = mat.mData[2][0];
			mjoint.globalTransform[9]  = mat.mData[2][1];
			mjoint.globalTransform[10] = mat.mData[2][2];
			mjoint.globalTransform[11] = mat.mData[2][3];

			mjoint.globalTransform[12] = mat.mData[3][0];
			mjoint.globalTransform[13] = mat.mData[3][1];
			mjoint.globalTransform[14] = mat.mData[3][2];
			mjoint.globalTransform[15] = mat.mData[3][3];
			moralesMesh.bindPose.push_back(mjoint);
		}

		std::cout << "Bind pose loaded, " << moralesMesh.bindPose.size() << " joints\n\n";
		std::cout << "Loading animation data...\n";

		// bind pose completed (hopefully)
		// Get the animation data

		// From the scene, get the animation stack

		FbxAnimStack* aStack = Scene->GetCurrentAnimationStack();

		// Get the duration of the animation

		FbxTimeSpan timeSpan = aStack->GetLocalTimeSpan();
		FbxTime time = timeSpan.GetDuration();

		ulong animationFrames = time.GetFrameCount(FbxTime::eFrames24);

		moralesMesh.animation.duration = time.GetSecondDouble();

		std::cout << "Animation duration: " << moralesMesh.animation.duration << " seconds\n";
		std::cout << "Animation frame count: " << animationFrames << " frames\n";

		for (ulong i = 0; i < animationFrames; i++)
		{
			MoralesKeyframe kf;

			time.SetFrame(i, FbxTime::eFrames24);
			kf.keytime = time.GetSecondDouble();
			// Still on step 8

			for (int j = 0; j < joints.size(); j++)
			{
				MoralesJoint mjoint;
				mjoint.parentIndex = joints[j].parentIndex;
				FbxAMatrix mat = joints[j].node->EvaluateGlobalTransform(time);

				ConvertFbxAMatrixToFloat16(mjoint.globalTransform, mat);

				kf.poseData.push_back(mjoint);
			}

			moralesMesh.animation.keyframes.push_back(kf);
		}

		std::cout << "Loading Vertex Skin Data\n";

		

		for (auto& inf : controlPointInfluences)
		{
			inf.infs[0].joint = -1;
			inf.infs[0].weight = -1.0;
			inf.infs[1].joint = -1;
			inf.infs[1].weight = -1.0;
			inf.infs[2].joint = -1;
			inf.infs[2].weight = -1.0;
			inf.infs[3].joint = -1;
			inf.infs[3].weight = -1.0;

		}

		int nodeCount = bindPose->GetCount();
		for (int i = 0; i < nodeCount; i++)
		{
			FbxNode* node = bindPose->GetNode(i);
			if (node != NULL)
			{
				FbxMesh* mesh = node->GetMesh();
				if (mesh != NULL)
				{
					numControlPoints = mesh->GetControlPointsCount();
					controlPointInfluences.resize(numControlPoints);

					int deformerCount = mesh->GetDeformerCount();
					for (int j = 0; j < deformerCount; j++)
					{
						FbxDeformer* deformer = mesh->GetDeformer(j);

						if (deformer != NULL && deformer->Is<FbxSkin>())
						{
							FbxSkin* skin = FbxCast<FbxSkin>(deformer);
							int clusterCount = skin->GetClusterCount();
							for (int k = 0; k < clusterCount; k++)
							{
								FbxCluster* cluster = skin->GetCluster(k);

								FbxNode* linkedNode = cluster->GetLink();
								for (int l = 0; l < joints.size(); ++l)
								{
									if (joints[l].node == linkedNode)
									{
										int joint_index = l;
										int controlPointsInCluster = cluster->GetControlPointIndicesCount();
										double* weights = cluster->GetControlPointWeights();
										int* point_indices = cluster->GetControlPointIndices();
										for (int I = 0; I < controlPointsInCluster; I++)
										{
											MoralesInfluence mi = { joint_index, weights[I] };

											AddAndKeepArraySorted(controlPointInfluences[point_indices[I]], mi);

										}
										break;
									}
								}
							}
						}
					}
				}
			}
		}

		std::cout << "Loaded control influences\n";

		std::cout << "Loaded Animation\n\n";


		// TODO: Get animation (skin?) weights

	}

	void AddAndKeepArraySorted(MoralesInfluenceSet& mis, MoralesInfluence& mi)
	{

		int size = MAX_INFLUENCES;
		for (int i = 0; i < MAX_INFLUENCES; i++)
		{
			if (mis.infs[i].joint == -1)
			{
				size = i;
				break;
			}
		}

		if (size < MAX_INFLUENCES)
		{
			mis.infs[size] = mi;
			return;
		}

		double lowest_weight = mi.weight;
		for (int i = 0; i < size; i++)
		{
			if (mis.infs[i].weight < lowest_weight)
			{
				mis.infs[MAX_INFLUENCES - 1] = mi;
				SortMIS(mis);
				return;
			}
		}
	}

	void SortMIS(MoralesInfluenceSet& mis) // slow bubble sort, could optimize later.
	{
		int size = MAX_INFLUENCES;
		for (int i = 0; i < MAX_INFLUENCES; i++)
		{
			if (mis.infs[i].joint == -1)
			{
				size = i;
				break;
			}
		}
		int moves;
		do 
		{
			moves = 0;
			for (int i = 0; i < size - 1; i++)
			{
				if (mis.infs[i].weight < mis.infs[i + 1].weight)
				{
					MoralesInfluence t = mis.infs[i];
					mis.infs[i] = mis.infs[i + 1];
					mis.infs[i + 1] = t;
					moves++;
				}
			}
		} while (moves != 0);
	}

	void ConvertFbxAMatrixToFloat16(float* m, const FbxAMatrix& mat)
	{
		m[0] = mat.mData[0][0];
		m[1] = mat.mData[0][1];
		m[2] = mat.mData[0][2];
		m[3] = mat.mData[0][3];

		m[4] = mat.mData[1][0];
		m[5] = mat.mData[1][1];
		m[6] = mat.mData[1][2];
		m[7] = mat.mData[1][3];

		m[8] = mat.mData[2][0];
		m[9] = mat.mData[2][1];
		m[10] = mat.mData[2][2];
		m[11] = mat.mData[2][3];

		m[12] = mat.mData[3][0];
		m[13] = mat.mData[3][1];
		m[14] = mat.mData[3][2];
		m[15] = mat.mData[3][3];
	}

	void SaveMesh(const char* meshFileName, MoralesMesh& mesh)
	{
		std::ofstream file(meshFileName, std::ios::trunc | std::ios::binary | std::ios::out);

		assert(file.is_open());

		uint32_t index_count = (uint32_t)mesh.indicesList.size();
		uint32_t vert_count = (uint32_t)mesh.vertexList.size();
		uint32_t mat_count = (uint32_t)mesh.materialList.size();
		uint32_t matp_count = (uint32_t)mesh.materialPaths.size();
		uint32_t bindpose_joint_count = (uint32_t)mesh.bindPose.size();
		uint32_t frame_count = (uint32_t)mesh.animation.keyframes.size();

		file.write((const char*)&index_count, sizeof(uint32_t));
		file.write((const char*)mesh.indicesList.data(), sizeof(uint32_t) * index_count);

		file.write((const char*)&vert_count, sizeof(uint32_t));
		file.write((const char*)mesh.vertexList.data(), sizeof(MoralesVertex) * vert_count);

		file.write((const char*)&mat_count, sizeof(uint32_t));
		file.write((const char*)mesh.materialList.data(), sizeof(MoralesMaterial) * mat_count);

		file.write((const char*)&matp_count, sizeof(uint32_t));
		// loop material pathes
		for (size_t i = 0; i < matp_count; i++)
		{
			uint32_t string_size = mesh.materialPaths[i].size();
			file.write((const char*)&string_size, sizeof(uint32_t));
			file.write(mesh.materialPaths[i].c_str(), sizeof(char) * string_size);
		}

		file.write((const char*)&bindpose_joint_count, sizeof(uint32_t));
		file.write((const char*)mesh.bindPose.data(), sizeof(MoralesJoint) * bindpose_joint_count);

		file.write((const char*)&mesh.animation.duration, sizeof(double));

		uint32_t joint_count = mesh.animation.keyframes[0].poseData.size();
		file.write((const char*)&joint_count, sizeof(uint32_t));
		file.write((const char*)&frame_count, sizeof(uint32_t));
		// loop keyframes
		for (size_t i = 0; i < frame_count; i++)
		{
			file.write((const char*)&mesh.animation.keyframes[i].keytime, sizeof(double));
			file.write((const char*)mesh.animation.keyframes[i].poseData.data(), sizeof(MoralesJoint) * joint_count);
		}

		file.close();
	}

	std::string ReplaceFBXExtension(std::string fileName)
	{
		fileName.replace(fileName.end() - 3, fileName.end(), "mbm");

		return fileName;
	}

	// Pretty basic implmentation of float equality. May be changed later.
	bool AreEqual(float a, float b)
	{
		//return fabs(a - b) <= std::numeric_limits<float>::epsilon();
		return a == b;
	}

	std::string OpenFileName(const wchar_t* filter, HWND owner)
	{
		OPENFILENAME ofn;
		wchar_t fileName[MAX_PATH] = L"";
		ZeroMemory(&ofn, sizeof(ofn));
		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner = owner;
		ofn.lpstrFilter = filter;
		ofn.lpstrFile = fileName;
		ofn.nMaxFile = MAX_PATH;
		ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
		ofn.lpstrDefExt = L"";
		std::wstring fileNameStr;
		if (GetOpenFileName(&ofn))
			fileNameStr = fileName;

		std::string returnString = std::string(fileNameStr.begin(), fileNameStr.end());

		return returnString;
	}
}

int main(int argc, char** argv)
{
	return MFBXExporter::main(argc, argv);
}