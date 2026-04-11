
#if UNITY_EDITOR
using UnityEngine;
using System.Collections;
using UnityEditor;
using System.IO;
using System.Collections.Generic;
using System;
using Random = UnityEngine.Random;
using System.Globalization;
using System.Text;

public class UTUUtilities : AssetPostprocessor
{
	class Section
	{
		public int Offset;
		public int NumIndices;
		public int Materialindex;
		public string MaterialName;
		public Material Mat;
	}
	class MeshBinding
	{
		public Mesh mesh;
		public MeshRenderer renderer;
	}
	void GetFloatsFromLine( string Line, ref List<float> Floats, GameObject gameobject )
	{
		Floats.Clear();

		string[] Tokens = Line.Split(new char[]{ ' ' } );

		for( int u = 1; u < Tokens.Length; u++ )
		{
			try
			{
				CultureInfo ci = (CultureInfo)CultureInfo.InvariantCulture.Clone();
				ci.NumberFormat.CurrencyDecimalSeparator = ".";
				float Val = float.Parse( Tokens[u], ci.NumberFormat);
				Floats.Add( Val );
			}
			catch( FormatException e )
			{
				Debug.LogError( "float.Parse failed in " + gameobject.name + " input=" + Tokens[u] + " exception=" + e.Message );
			}
		}
	}
	void GetIntsFromLine( string Line, ref List<int> Ints, GameObject gameobject )
	{
		Ints.Clear();

		string[] Tokens = Line.Split(new char[]{ ' ', '/' } );

		for( int u = 1; u < Tokens.Length; u++ )
		{
			try
			{
				int Val = int.Parse( Tokens[u] );
				Ints.Add( Val );
			}
			catch( FormatException e )
			{
				Debug.LogError( "int.Parse failed in " + gameobject.name + " input=" + Tokens[u] + " exception=" + e.Message );
			}
		}
	}
	private void OnPostprocessModel( GameObject gameobject )
	{
		UnityEditor.ModelImporter importer = this.assetImporter as UnityEditor.ModelImporter;

		string Path = importer.assetPath;
		if( !Path.Contains( ".obj" ) )
			return;


		string OBJFile = File.ReadAllText( Path );

		List<float> Floats = new List<float>();
		List<int> Ints = new List<int>();

		List<Vector3> Vertices = new List<Vector3>();
		List<Color> Colors = new List<Color>();
		List<Vector2> Texcoords0 = new List<Vector2>();
		List<Vector2> Texcoords1 = new List<Vector2>();
		List<Vector3> Normals = new List<Vector3>();
		List<Vector4> Tangents = new List<Vector4>();
		bool ReplaceNormals = true;
		List<int> Indices = new List<int>();
		List<Section> Sections = new List<Section>();
		string[] Lines = OBJFile.Split( new char[]{ '\n' } );
		for( int i = 0; i < Lines.Length; i++ )
		{
			string Line = Lines[i];
			if( Line.StartsWith( "v " ) )
			{
				string[] Tokens = Line.Split(new char[]{ ' ' } );
				Floats.Clear();
				for( int u = 1; u < Tokens.Length; u++ )//0 should be "v"
				{
					try
					{
						CultureInfo ci = (CultureInfo)CultureInfo.InvariantCulture.Clone();
						ci.NumberFormat.CurrencyDecimalSeparator = ".";
						float Val = float.Parse( Tokens[u], ci.NumberFormat);
						Floats.Add( Val );
					}
					catch( FormatException e )
					{
						Debug.LogError( "float.Parse failed for verts in " + gameobject.name + " input=" + Tokens[u] + " exception=" + e.Message );
					}
				}

				if( Floats.Count >= 3 )
					Vertices.Add( new Vector3( Floats[0], Floats[1], Floats[2] ) );
				else
					Vertices.Add( new Vector3( 0, 0, 0 ) );

				if( Floats.Count == 7 )
				{
					Colors.Add( new Color( Floats[3], Floats[4], Floats[5], Floats[6] ) );
				}
			}
			else if( Line.StartsWith( "vt " ) )
			{
				GetFloatsFromLine( Line, ref Floats, gameobject );
				if( Floats.Count == 2 )
				{
					Texcoords0.Add( new Vector2( Floats[0], Floats[1] ) );
				}
				else
					Texcoords0.Add( new Vector2( 0, 0 ) );
			}
			else if( Line.StartsWith( "vt1 " ) )
			{
				GetFloatsFromLine( Line, ref Floats, gameobject );
				if( Floats.Count == 2 )
				{
					Texcoords1.Add( new Vector2( Floats[0], Floats[1] ) );
				}
				else
					Texcoords1.Add( new Vector2( 0, 0 ) );
			}
			else if( ReplaceNormals && Line.StartsWith( "vn " ) )
			{
				GetFloatsFromLine( Line, ref Floats, gameobject );
				if( Floats.Count == 3 )
				{
					Normals.Add( new Vector3( Floats[0], Floats[1], Floats[2] ) );
				}
				else
					Normals.Add( new Vector3( 0, 0, 0 ) );
			}
			else if( Line.StartsWith( "tan " ) )
			{
				GetFloatsFromLine( Line, ref Floats, gameobject );
				if( Floats.Count == 4 )
				{
					Tangents.Add( new Vector4( Floats[0], Floats[1], Floats[2], Floats[3] ) );
				}
				else
					Normals.Add( new Vector4( 0, 0, 0, 0 ) );
			}
			else if( Line.StartsWith( "f " ) )
			{
				GetIntsFromLine( Line, ref Ints, gameobject );
				if( Ints.Count == 9 )
				{
					Indices.Add( Ints[0] - 1 );
					Indices.Add( Ints[3] - 1 );
					Indices.Add( Ints[6] - 1 );
				}
			}
			//"#Section %d Offset %d NumIndices %d MaterialIndex %d %s\n"
			if( Line.StartsWith( "#Section " ) )
			{
				string[] Tokens = Line.Split(new char[]{ ' ' } );
				if( Tokens.Length == 9 )
				{
					Section NewSection = new Section();
					NewSection.Offset = int.Parse( Tokens[3] );
					NewSection.NumIndices = int.Parse( Tokens[5] );
					//NewSection.Materialindex = int.Parse( Tokens[7] );
					NewSection.MaterialName = Tokens[8].Trim();
					Sections.Add( NewSection );
				}
			}
		}

		MeshBinding[] MeshArray = GetMeshes( gameobject);
		if( MeshArray.Length == 0 )
			return;

		MeshBinding Binding = MeshArray[0];
		Mesh m = Binding.mesh;

		if( m.vertices.Length != Vertices.Count )
		{
			Debug.LogError( "m.vertices.Length != Vertices.Count for " + gameobject.name );
		}
		if( m.GetIndices( 0 ).Length != Indices.Count )
		{
			Debug.LogError( "m.GetIndices( 0 ).Length != Indices.Count for " + gameobject.name );
		}
		m.SetIndices( Indices.ToArray(), MeshTopology.Triangles, 0 );
		m.vertices = Vertices.ToArray();
		if( Texcoords0.Count != m.uv.Length )
		{
			Debug.LogError( "Texcoords0.Count != m.uv.Length for " + gameobject.name );
		}
		m.uv = Texcoords0.ToArray();

		if( Colors.Count > 0 )
		{
			if( m.vertices.Length != Colors.Count )
			{
				Debug.LogError( " Mesh vertex count != color count for " + gameobject.name );
			}
			else
			{
				m.colors = Colors.ToArray();
			}
		}
		if( Texcoords1.Count > 0 )
		{
			if( m.vertices.Length != Texcoords1.Count )
			{
				Debug.LogError( " Mesh vertex count != Texcoords1 count for " + gameobject.name );
			}
			else
			{
				m.uv2 = Texcoords1.ToArray();
			}
		}

		if( Sections.Count > 1 )
		{
			SetupSubmeshes( m, Sections.ToArray(), gameobject.name );
		}

		Material[] UsedMaterials = GetSectionMaterials( Sections.ToArray() );
		Binding.renderer.sharedMaterials = UsedMaterials;

		if( ReplaceNormals && Normals.Count > 0 )
		{
			if( m.vertices.Length != Normals.Count )
			{
				Debug.LogError( " Mesh vertex count != Texcoords1 count " + gameobject.name );
			}
			else
			{
				m.normals = Normals.ToArray();
			}
		}
		if( Tangents.Count > 0 )
		{
			if( m.tangents.Length == 0 || m.tangents.Length == Tangents.Count )
			{
				m.tangents = Tangents.ToArray();
			}
			else
			{
				Debug.LogError( " Mesh vertex count != Tangents count " + gameobject.name );
			}
		}
		m.RecalculateBounds();
	}
	Material[] GetSectionMaterials( Section[] sections )
	{
		Material[] AllMaterials = UnityEngine.Resources.FindObjectsOfTypeAll<Material>();
		Material[] UsedMaterials = new Material[sections.Length];

		for( int i = 0; i < sections.Length; i++ )
		{
			Section s = sections[i];

			if( !s.MaterialName.Equals( "None" ) )
			{
				for( int m = 0; m < AllMaterials.Length; m++ )
				{
					if( AllMaterials[m].name.Equals( s.MaterialName ) )
					{
						s.Mat = AllMaterials[m];
						UsedMaterials[i] = s.Mat;
						break;
					}
				}
			}
		}

		return UsedMaterials;
	}
	void SetupSubmeshes( Mesh mesh, Section[] sections, String Name )
	{
		int[] AllIndices = mesh.GetIndices( 0 );
		mesh.subMeshCount = sections.Length;

		for( int i = 0; i < sections.Length; i++ )
		{
			Section s = sections[i];

			int[] SectionIndices = new int[ s.NumIndices ];

			for( int u = 0; u < s.NumIndices; u++ )
			{
				if( s.Offset + u < AllIndices.Length )
					SectionIndices[u] = AllIndices[s.Offset + u];
				else
				{
					Debug.LogError( "AllIndices is too small for " + Name );
					break;
				}
			}

			mesh.SetIndices( SectionIndices, MeshTopology.Triangles, i );
		}
	}
	MeshBinding[] GetMeshes( GameObject gameObject )
	{
		List<MeshBinding> MeshArray = new List<MeshBinding>();
		MeshFilter[] MFArray = gameObject.GetComponentsInChildren<MeshFilter>();
		if( MFArray != null )
		{
			for( int i = 0; i < MFArray.Length; i++ )
			{
				MeshFilter MF = MFArray[i];
				Mesh m = MFArray[i].sharedMesh;

				MeshBinding NewMeshBinding = new MeshBinding();
				NewMeshBinding.mesh = m;
				NewMeshBinding.renderer = MF.gameObject.GetComponent<MeshRenderer>();
				MeshArray.Add( NewMeshBinding );
			}
		}

		return MeshArray.ToArray();
	}
	[MenuItem( "UTU/PrintMesh" )]
	static void PrintMesh()
	{
		if( Selection.gameObjects.Length == 0 )
			return;
		GameObject go = Selection.gameObjects[0];

		MeshFilter MF = go.GetComponent<MeshFilter>();
		Mesh mesh = MF.sharedMesh;

		StringBuilder Data = new StringBuilder();
		Data.Append("Vertices " + mesh.vertexCount + "\n");

		
		for( int i = 0; i < mesh.subMeshCount; i++ )
		{
			int[] Indices = mesh.GetTriangles( i );

			uint BaseVertex = mesh.GetBaseVertex( i );
			Data.Append( "Section " + i + " Indices " + Indices.Length + " BaseVertex " + BaseVertex + "\n");
		}

		Vector3[] vertices = mesh.vertices;
		for( int i = 0; i < vertices.Length; i++ )
		{
			Data.Append( "v " + vertices[i].x + " " + vertices[i].y + " " + vertices[i].z + "\n" );
		}

		Vector2[] uv = mesh.uv;
		for( int i = 0; i < vertices.Length; i++ )
		{
			Data.Append( "uv " + uv[i].x + " " + uv[i].y + "\n" );
		}

		for( int i = 0; i < mesh.subMeshCount; i++ )
		{
			int[] Indices = mesh.GetTriangles( i );
			Data.Append( "Section " + i + "\n" );
			for( int u = 0; u < Indices.Length; u += 3 )
			{
				int I1 = Indices[ u ]    + 1;
				int I2 = Indices[ u + 1] + 1;
				int I3 = Indices[ u + 2] + 1;
				//Debug.Log( "f " + I1 + " " + I2 + " " + I3 );
				Data.Append( "f " + I3 + " " + I2 + " " + I1 + "\n" );
			}
		}

		File.WriteAllText( "C:/UnrealToUnity/Out.txt", Data.ToString() );
	}
	[MenuItem( "UTU/DisableAllLights" )]
	static void DisableAllLights()
	{
		UnityEngine.Object[] AllLights = Resources.FindObjectsOfTypeAll( typeof( Light ) );

		for(int i=0; i< AllLights.Length; i++ )
        {
			Light L = AllLights[i] as Light;
			L.enabled = false;
		}
	}
	[MenuItem( "UTU/ConvertRectLightsToPointLights" )]
	static void ConvertRectLightsToPointLights()
	{
		UnityEngine.Object[] AllLights = Resources.FindObjectsOfTypeAll( typeof( Light ) );

		for( int i = 0; i < AllLights.Length; i++ )
		{
			Light L = AllLights[i] as Light;
			if( L.type == LightType.Rectangle )
			{
				L.type = LightType.Point;
				L.intensity = Math.Clamp( L.intensity, 0.0f, 3.0f );
			}
		}
	}
	[MenuItem( "UTU/DisableLocalLightShadows" )]
	static void DisableLocalLightShadows()
	{
		UnityEngine.Object[] AllLights = Resources.FindObjectsOfTypeAll( typeof( Light ) );

		for( int i = 0; i < AllLights.Length; i++ )
		{
			Light L = AllLights[i] as Light;
			if( L.type != LightType.Directional && L.shadows != LightShadows.None)
			{
				L.shadows = LightShadows.None;
			}
		}
	}
	[MenuItem( "UTU/SetBounds" )]
	static void SetBounds()
	{
		MeshRenderer[] AllMeshRenderers = Resources.FindObjectsOfTypeAll<MeshRenderer>();

		for(int i=0; i< AllMeshRenderers.Length; i++ )
        {
			MeshRenderer MR = AllMeshRenderers[i];
			if( MR == null )
				continue;

			MeshFilter MF = MR.gameObject.GetComponent<MeshFilter>();
			if( MF == null || MF.sharedMesh == null )
				return;
					
			Vector3 LocalObjectBoundsMin = MF.sharedMesh.bounds.min;
			Vector3 LocalObjectBoundsMax = MF.sharedMesh.bounds.max;

			bool GlobalSpace = false;
			if ( GlobalSpace )
            {
				LocalObjectBoundsMin = MR.bounds.min;
				LocalObjectBoundsMax = MR.bounds.max;
			}

			//To Unreal units
			LocalObjectBoundsMin.Set( LocalObjectBoundsMin.x * 100, LocalObjectBoundsMin.z * 100, LocalObjectBoundsMin.y * 100 );
			LocalObjectBoundsMax.Set( LocalObjectBoundsMax.x * 100, LocalObjectBoundsMax.z * 100, LocalObjectBoundsMax.y * 100 );

			Material[] Mats = MR.sharedMaterials;
			if( Mats != null )
			{
				for( int u = 0; u < Mats.Length; u++ )
				{
					Material mat = Mats[u];
					mat.SetVector( "LocalObjectBoundsMin", LocalObjectBoundsMin );
					mat.SetVector( "LocalObjectBoundsMax", LocalObjectBoundsMax );
				}
			}
		}
	}
	static Vector3 GetPosition( Matrix4x4 mat )
    {
		return new Vector3( mat.m03, mat.m13, mat.m23 );
    }
	static GameObject AddLodGroupToPrefabRoot( GameObject Prefab )
	{
		LODGroup Lods = Prefab.GetComponent<LODGroup>();
		if( Lods == null )
		{
			//Add a lodgroup manually since objects with a single LOD won't get rendered by the terrain system
			Lods = Prefab.AddComponent<LODGroup>();
			MeshRenderer MR = Prefab.GetComponentInChildren<MeshRenderer>();
			if( MR != null )
			{
				LOD NewLOD = new LOD();
				NewLOD.renderers = new Renderer[] { MR };
				LOD[] AllLods = new LOD[]{ NewLOD };
				Lods.SetLODs( AllLods );
			}

			return PrefabUtility.SavePrefabAsset( Prefab );
		}
		else
			return Prefab;
	}
	[MenuItem( "UTU/MoveMeshesToTerrainComponent" )]
	static void MoveMeshesToTerrainComponent()
	{
		bool InitializedBounds = false;
		Bounds WorldBounds = new Bounds(new Vector3(0,0,0), new Vector3(1,1,1) );
		GameObject[] AllGameObjects = UnityEngine.Object.FindObjectsOfType<GameObject>();
		List<GameObject> PrefabsList = new List<GameObject>();
		List<GameObject> PrefabInstanceList = new List<GameObject>();
		List<GameObject> UniquePrefabs = new List<GameObject>();
		for( int u = 0; u < AllGameObjects.Length; u++ )
		{
			var GO = AllGameObjects[u];
			MeshRenderer MR = GO.GetComponent<MeshRenderer>();
			if ( MR != null )
            {
				if( !InitializedBounds )
				{
					WorldBounds = MR.bounds;
					InitializedBounds = true;
				}
				else
                {
					WorldBounds.Encapsulate( MR.bounds );
				}
			}
			var type = PrefabUtility.GetPrefabAssetType( GO );
			if( type == PrefabAssetType.Regular )
			{
				GameObject PrefabSource = PrefabUtility.GetCorrespondingObjectFromSource( GO );
				//Only check prefab roots
				if( PrefabSource.transform.parent == null )
				{
					PrefabsList.Add( PrefabSource );
					PrefabInstanceList.Add( GO );
					if( UniquePrefabs.IndexOf( PrefabSource ) == -1 )
					{
						UniquePrefabs.Add( PrefabSource );
					}
				}
			}
		}

		Terrain[] AllTerrains = UnityEngine.Object.FindObjectsOfType<Terrain>();

		if( AllTerrains.Length == 0 )
		{
			TerrainData NewTerrainData = new TerrainData();
			NewTerrainData.size = WorldBounds.extents * 2;
			GameObject TerrainGO = Terrain.CreateTerrainGameObject( NewTerrainData );
			TerrainGO.transform.position = WorldBounds.center - WorldBounds.extents;

			Terrain NewTerrain = TerrainGO.GetComponent<Terrain>();
			AllTerrains = new Terrain[1];
			//Terrain NewTerrain = new Terrain();

			//NewTerrain.terrainData = new TerrainData();

			TreePrototype[] Prototypes = new TreePrototype[UniquePrefabs.Count];
			for(int i=0; i< UniquePrefabs.Count; i++)
            {
				Prototypes[i] = new TreePrototype();
				Prototypes[i].prefab = AddLodGroupToPrefabRoot( UniquePrefabs[i] );
			}
			NewTerrainData.treePrototypes = Prototypes;
			AllTerrains[0] = NewTerrain;
		}

		Terrain  SelectedTerrain = AllTerrains[0];
		TreePrototype[] TreePrototypes = SelectedTerrain.terrainData.treePrototypes;

		TreeInstance[] CurrentInstances = SelectedTerrain.terrainData.treeInstances;
		List<TreeInstance> CurrentInstancesList = new List<TreeInstance>();
		for(int i=0; i< CurrentInstances.Length; i++ )
        {
			CurrentInstancesList.Add( CurrentInstances[i] );
		}

		bool Quit = false;
		if( Quit )
			return;

		for(int i=0; i< TreePrototypes.Length; i++ )
        {
			var Prototype = TreePrototypes[ i ];

			if( Prototype.prefab == null )
				continue;

			for(int u=0; u< PrefabsList.Count; u++)
            {
				var PrefabSource = PrefabsList[u];
				GameObject GO = PrefabInstanceList[u];
				if( PrefabSource == Prototype.prefab )
				{
					TreeInstance NewInstance = new TreeInstance();
					NewInstance.prototypeIndex = i;

					//Move Gameobject in terrain space
					Matrix4x4 Mat = GO.transform.localToWorldMatrix;
					Matrix4x4 TerrainMatInv = SelectedTerrain.gameObject.transform.localToWorldMatrix.inverse;// * SelectedTerrain.gameObject.transform.
					Matrix4x4 FinalMat = TerrainMatInv * Mat;
					Vector3 FinalPosition = GetPosition( FinalMat );
						
					//var InTerrainSpace = GO.transform.localToWorldMatrix * SelectedTerrain.gameObject.transform;
					float X = FinalPosition.x / SelectedTerrain.terrainData.size.x;
					float Y = FinalPosition.y / SelectedTerrain.terrainData.size.y;
					float Z = FinalPosition.z / SelectedTerrain.terrainData.size.z;
					NewInstance.position = new Vector3( X, Y, Z );
					float OneDegreeAngle = ((float)Math.PI) / 180.0f;
					NewInstance.rotation = FinalMat.rotation.eulerAngles.y * OneDegreeAngle;
					NewInstance.heightScale = GO.transform.localScale.y;
					NewInstance.widthScale = ( GO.transform.localScale.x + GO.transform.localScale.z ) / 2;
					NewInstance.color = NewInstance.lightmapColor = new Color32( 255, 255, 255, 255 );
					CurrentInstancesList.Add( NewInstance );

					bool DeleteGameObject = true;
					if ( DeleteGameObject )
                    {
						UnityEngine.Object.DestroyImmediate( GO );
					}
				}
			}			
		}

		SelectedTerrain.terrainData.treeInstances = CurrentInstancesList.ToArray();
	}
	
	[MenuItem( "UTU/MoveTreeDataToTerrain" )]
	static void MoveTreeDataToTerrain()
	{
		bool InitializedBounds = false;
		Bounds WorldBounds = new Bounds(new Vector3(0,0,0), new Vector3(1,1,1) );
		GameObject[] AllGameObjects = UnityEngine.Object.FindObjectsOfType<GameObject>();
		for( int u = 0; u < AllGameObjects.Length; u++ )
		{
			var GO = AllGameObjects[u];
			MeshRenderer MR = GO.GetComponent<MeshRenderer>();
			if( MR != null )
			{
				if( !InitializedBounds )
				{
					WorldBounds = MR.bounds;
					InitializedBounds = true;
				}
				else
				{
					WorldBounds.Encapsulate( MR.bounds );
				}
			}
		}

		Terrain[] AllTerrains = UnityEngine.Object.FindObjectsOfType<Terrain>();

		if( AllTerrains.Length == 0 )
		{
			TerrainData NewTerrainData = new TerrainData();
			NewTerrainData.size = WorldBounds.extents * 2;
			GameObject TerrainGO = Terrain.CreateTerrainGameObject( NewTerrainData );
			TerrainGO.transform.position = WorldBounds.center - WorldBounds.extents;

			Terrain NewTerrain = TerrainGO.GetComponent<Terrain>();
			AllTerrains = new Terrain[1];
			AllTerrains[0] = NewTerrain;
		}

		Terrain  SelectedTerrain = AllTerrains[0];
		TreePrototype[] TreePrototypes = SelectedTerrain.terrainData.treePrototypes;
		TreeInstanceComponent[] TreeInstanceComponents = UnityEngine.Object.FindObjectsOfType<TreeInstanceComponent>();
		if( TreeInstanceComponents.Length == 0 )
			return;

		TreeInstanceComponent TreeInstances = TreeInstanceComponents[0];

		TreeInstance[] CurrentInstances = SelectedTerrain.terrainData.treeInstances;
		List<TreeInstance> CurrentInstancesList = new List<TreeInstance>();
		List<TreePrototype> TreePrototypesList = new List<TreePrototype>();
		for( int i = 0; i < CurrentInstances.Length; i++ )
		{
			CurrentInstancesList.Add( CurrentInstances[i] );
		}

		bool Quit = false;
		if( Quit )
			return;

		for( int i = 0; i < TreeInstances.TreePrefabs.Count; i++ )
		{
			GameObject TreePrefab = TreeInstances.TreePrefabs[ i ];

			if( TreePrefab == null )
				continue;

			GameObject SavedPrefab = AddLodGroupToPrefabRoot( TreePrefab );
			if( SavedPrefab )
				TreePrefab = SavedPrefab;

			TreePrototype Prototype = null;
			int prototypeIndex = -1;
			for(int u=0; u< TreePrototypesList.Count; u++)
            {
				if ( TreePrototypesList[u].prefab == TreePrefab)
                {
					Prototype = TreePrototypesList[u];
					prototypeIndex = u;
					break;
				}
            }

			if ( Prototype  == null)
            {
				Prototype = new TreePrototype();
				Prototype.prefab = TreePrefab;
				prototypeIndex = TreePrototypesList.Count;
				TreePrototypesList.Add( Prototype );
			}

			TreeInstance NewInstance = new TreeInstance();
			NewInstance.prototypeIndex = prototypeIndex;

			Quaternion Quat = Quaternion.Euler( new Vector3( 0, TreeInstances.rotation[i], 0 ) );
			Matrix4x4 WorldMatrix = Matrix4x4.TRS( TreeInstances.position[i], Quat, new Vector3( TreeInstances.widthScale[i], TreeInstances.heightScale[i], TreeInstances.widthScale[i] ) );

			//Move Gameobject in terrain space
			Matrix4x4 Mat = WorldMatrix;
			Matrix4x4 TerrainMatInv = SelectedTerrain.gameObject.transform.localToWorldMatrix.inverse;// * SelectedTerrain.gameObject.transform.
			Matrix4x4 FinalMat = TerrainMatInv * Mat;
			Vector3 FinalPosition = GetPosition( FinalMat );

			//var InTerrainSpace = GO.transform.localToWorldMatrix * SelectedTerrain.gameObject.transform;
			float X = FinalPosition.x / SelectedTerrain.terrainData.size.x;
			float Y = FinalPosition.y / SelectedTerrain.terrainData.size.y;
			float Z = FinalPosition.z / SelectedTerrain.terrainData.size.z;
			NewInstance.position = new Vector3( X, Y, Z );
			float OneDegreeAngle = ((float)Math.PI) / 180.0f;
			NewInstance.rotation = FinalMat.rotation.eulerAngles.y * OneDegreeAngle;
			NewInstance.heightScale = TreeInstances.heightScale[i];
			NewInstance.widthScale = TreeInstances.widthScale[i];
			NewInstance.color = NewInstance.lightmapColor = new Color32( 255, 255, 255, 255 );
			CurrentInstancesList.Add( NewInstance );					
		}

		SelectedTerrain.terrainData.treePrototypes = TreePrototypesList.ToArray();
		SelectedTerrain.terrainData.treeInstances = CurrentInstancesList.ToArray();		
	}
	static bool IdenticalMeshMaterials( MeshFilter MFA, MeshFilter MFB, MeshRenderer MRA, MeshRenderer MRB)
    {
		if( MFA == null || MFB == null || MRA == null || MRB == null )
			return false;
		if( MFA.sharedMesh == MFB.sharedMesh && MRA.sharedMaterials.Length == MRB.sharedMaterials.Length )
		{
			for( int m = 0; m < MRA.sharedMaterials.Length; m++ )
			{
				if( MRA.sharedMaterials[m] != MRB.sharedMaterials[m] )
				{
					return false;
				}
			}

			return true;
		}
		else
			return false;
	}
	static int GetLODLevel( LODGroup[] AllLodGroups, MeshRenderer MR )
    {
		for( int i = 0; i < AllLodGroups.Length; i++ )
		{
			var LGA = AllLodGroups[i];
			LOD[] Lods = LGA.GetLODs();
			for(int u=0; u<Lods.Length; u++ )
            {
				for(int r= 0; r< Lods[u].renderers.Length; r++)
                {
					if( Lods[u].renderers[r] == MR )
						return u;
                }
            }
		}

		return -1;
	}
	static void DisableMeshRenderer( LODGroup[] AllLodGroups, MeshRenderer MR )
    {
		MR.enabled = false;
		int Level = GetLODLevel( AllLodGroups, MR );
		//Disable all LOD levels
		if ( Level != -1 )
        {
			LODGroup LG = MR.gameObject.GetComponentInParent<LODGroup>();
			LOD[] Lods = LG.GetLODs();
			for( int u = 0; u < Lods.Length; u++ )
			{
				for( int r = 0; r < Lods[u].renderers.Length; r++ )
				{
					Lods[u].renderers[r].enabled = false;
				}
			}
		}
	}
	[MenuItem( "UTU/CreateInstancedMeshComponents" )]
	static void CreateInstancedMeshComponents()
	{
		LODGroup[] AllLodGroups = UnityEngine.Object.FindObjectsOfType<LODGroup>();
		for( int i = 0; i < AllLodGroups.Length; i++ )
		{
			var LGA = AllLodGroups[i];
		}

		GameObject[] AllGameObjects = UnityEngine.Object.FindObjectsOfType<GameObject>();
		List<GameObject> ValidGameObjects = new List<GameObject>();

		for( int i = 0; i < AllGameObjects.Length; i++ )
		{
			var GO = AllGameObjects[i];
			MeshRenderer MRA = GO.GetComponent<MeshRenderer>();
			int Level = GetLODLevel( AllLodGroups, MRA );
			if( Level == -1 || Level == 0 )
				ValidGameObjects.Add( GO );
		}

		for( int i = 0; i < ValidGameObjects.Count - 1; i++ )
		{
			var ObjA = ValidGameObjects[i];

			MeshFilter MFA = ObjA.GetComponent<MeshFilter>();
			MeshRenderer MRA = ObjA.GetComponent<MeshRenderer>();
			List<Transform> TransformList = new List<Transform>();
			if( MFA == null || MRA == null || !MRA.enabled)
				continue;

			
			for( int u = i + 1; u < ValidGameObjects.Count; u++ )
			{
				var ObjB = ValidGameObjects[u];

				MeshFilter MFB = ObjB.GetComponent<MeshFilter>();
				MeshRenderer MRB = ObjB.GetComponent<MeshRenderer>();

				bool Identical = IdenticalMeshMaterials( MFA, MFB, MRA,MRB);
				if( !Identical )
					continue;

				DisableMeshRenderer( AllLodGroups, MRB );
				TransformList.Add( ObjB.transform );
			}

			if( TransformList.Count > 0 )
			{
				TransformList.Add( ObjA.transform );
				InstancedMesh ISM = ObjA.AddComponent<InstancedMesh>();

				for( int u = 0; u < TransformList.Count; u++ )
                {
					Transform T = TransformList[u];
					ISM.Translations.Add( T.position );
					ISM.Rotations.Add( T.rotation );
					ISM.Scales.Add( T.lossyScale );
				}

				DisableMeshRenderer( AllLodGroups, MRA );
			}
		}
	}
	class PathAndMaterials
    {
		public string Path;
		public Material[] Materials;
    }
	[MenuItem( "UTU/FixSkinnedMeshes" )]
	static void FixSkinnedMeshes()
	{
		AnimationClip[] AllAnimations = UnityEngine.Resources.FindObjectsOfTypeAll<AnimationClip>();
		SkinnedMeshRenderer[] AllSMR = UnityEngine.Object.FindObjectsOfType<SkinnedMeshRenderer>();
		LODGroup[] AllLodGroups = UnityEngine.Object.FindObjectsOfType<LODGroup>();

		List<PathAndMaterials> UniqueAssets = new List<PathAndMaterials>();

		for( int i = 0; i < AllLodGroups.Length; i++ )
		{
			LODGroup Group = AllLodGroups[i];
			LOD[] lods = Group.GetLODs();
			for(int l=0; l<lods.Length; l++ )
            {
				if ( lods[l].renderers.Length > 0)
                {
					SkinnedMeshRenderer IsSMR = lods[l].renderers[0] as SkinnedMeshRenderer;
					if ( IsSMR )
                    {
						var path = AssetDatabase.GetAssetPath(IsSMR.sharedMesh);
						PathAndMaterials FoundAsset = UniqueAssets.Find( Element => Element.Path.Equals( path) );

						if( FoundAsset == null )
						{
							PathAndMaterials NewPathAndMaterials = new PathAndMaterials();
							NewPathAndMaterials.Path = path;
							NewPathAndMaterials.Materials = IsSMR.sharedMaterials;
							UniqueAssets.Add( NewPathAndMaterials );
							break;
						}
					}
				}
            }
		}
		for( int i = 0; i < AllSMR.Length; i++ )
		{
			SkinnedMeshRenderer SMR = AllSMR[i];
			if( SMR == null || SMR.sharedMesh == null )
				continue;

			var path = AssetDatabase.GetAssetPath(SMR.sharedMesh);
			PathAndMaterials FoundAsset = UniqueAssets.Find( Element => Element.Path.Equals( path) );

			if( FoundAsset == null )
			{
				PathAndMaterials NewPathAndMaterials = new PathAndMaterials();
				NewPathAndMaterials.Path = path;
				NewPathAndMaterials.Materials = SMR.sharedMaterials;
				UniqueAssets.Add( NewPathAndMaterials );
				//break;
			}
		}

		for( int i = 0; i < UniqueAssets.Count; i++ )
		{
			PathAndMaterials Asset = UniqueAssets[i];
			UnityEngine.Object mainAssetFile = AssetDatabase.LoadMainAssetAtPath(Asset.Path);
			GameObject RootGO = UnityEngine.Object.Instantiate( mainAssetFile, new Vector3( 0, 0, 0 ), Quaternion.identity ) as GameObject;

			SkinnedMeshRenderer[] InstancesSMR = RootGO.GetComponentsInChildren<SkinnedMeshRenderer>();
			for( int s = 0; s < InstancesSMR.Length; s++ )
			{
				InstancesSMR[s].sharedMaterials = Asset.Materials;
			}

			Animation AnimComp = RootGO.AddComponent<Animation>();

			if( AllAnimations.Length > 1 )
			{
				AnimComp.clip = AllAnimations[1];
			}
		}
	}
    static GameObject GetRendererObject( GameObject GO )
	{
		MeshRenderer[] Renderers = GO.GetComponentsInChildren<MeshRenderer>();
		if( Renderers != null && Renderers[0] != null )
			return Renderers[0].gameObject;

		return null;
    }

	[MenuItem( "UTU/ImportTerrainHeightmapAndGrass" )]
	static void ImportTerrainHeightmapAndGrass()
	{
        TreeInstanceComponent[] TreeInstanceComponents = UnityEngine.Object.FindObjectsOfType<TreeInstanceComponent>();
        if( TreeInstanceComponents.Length == 0 )
            return;

        TreeInstanceComponent TIC = TreeInstanceComponents[0];
		GameObject Landscape = TIC.gameObject;
        Terrain[] AllTerrains = UnityEngine.Object.FindObjectsOfType<Terrain>();
        
		string TexturePath = "Assets/" + TIC.HeightMapTexture;
        UnityEngine.Object Obj = AssetDatabase.LoadAssetAtPath<Texture>( TexturePath );
		if( Obj == null )
			return;
        Texture2D Tex = Obj as Texture2D;
        if( !Tex.isReadable )
        {
            Debug.LogError( "Tex " + Tex.ToString() + " is not readable !" );
            return;
        }
        if( AllTerrains.Length == 0 )
        {
            TerrainData NewTerrainData = new TerrainData();
            //NewTerrainData.size = Landscape.transform.localScale;

            NewTerrainData.size = new Vector3( Tex.width/2, TIC.TerrainHeight, Tex.height/2 );
            NewTerrainData.heightmapResolution = Tex.width;
            NewTerrainData.alphamapResolution = Tex.width;
            NewTerrainData.baseMapResolution = Tex.width;
			NewTerrainData.SetDetailResolution( Tex.width, 8 );// Tex.width, Tex.width );

            GameObject TerrainGO = Terrain.CreateTerrainGameObject( NewTerrainData );
            TerrainGO.transform.position = Landscape.transform.position;
            TerrainGO.transform.rotation = Landscape.transform.rotation;

            Terrain NewTerrain = TerrainGO.GetComponent<Terrain>();
            AllTerrains = new Terrain[1];
            AllTerrains[0] = NewTerrain;
        }

        if ( Tex != null )
		{
            int xBase = 0;
			int yBase = 0;
            Color[] Pixels = Tex.GetPixels();
			Color32[] Pixels32 = Tex.GetPixels32();
            float[,] heights = new float[ Tex.width + 1, Tex.height + 1];
			bool MirrorX = false;
            bool MirrorY = false;
			//Need to compute lowest because Unity doesn't work with negative heights
			float Lowest = 0.5f;
            for( int x = 0; x < Tex.width; x++ )
                for( int y = 0; y < Tex.height; y++ )
                {
                    Color c = Pixels[y * Tex.width + x ];
                    Color32 c32 = Pixels32[y * Tex.width + x ];
                    int Num16 = c32.r * 255 + c32.g;
                    float ValIn16 = (float)Num16 / 65535.0f;
					if( ValIn16 < Lowest )
						Lowest = ValIn16;
                }
            for(int x = 0; x <= Tex.width; x++ )
                for( int y = 0; y <= Tex.height; y++ )
				{
					int LoopX = x;
                    int LoopY = y;
					//Duplicate last vert because heightmap in unity is like 33x33,65x65,etc
					if ( x == Tex.width )
					{
						LoopX--;
                    }
                    if( y == Tex.height )
                    {
                        LoopY--;
                    }
                    Color c = Pixels[LoopY * Tex.width + LoopX ];
                    Color32 c32 = Pixels32[LoopY * Tex.width + LoopX ];
					int Num16 = c32.r * 255 + c32.g;
					float ValIn16 = (float)Num16 / 65535.0f;
                    float h = ValIn16;// c.r;
					int ActualX = x;
                    int ActualY = y;
                    if ( MirrorX )
					{
						ActualX = ( Tex.width ) - x;
                    }
                    if( MirrorY )
                    {
                        ActualY = ( Tex.height ) - y;
                    }
					heights[ActualX, ActualY] = ( h - Lowest );// - Lowest;
                }
            AllTerrains[0].terrainData.SetHeights( xBase, yBase, heights );            
        }

		DetailPrototype[] DetailPrototypes = new DetailPrototype[TIC.GrassPrefabs.Count];

		for( int i = 0; i < TIC.GrassPrefabs.Count; i++ )
		{
			GameObject Prefab = TIC.GrassPrefabs[i];
			DetailPrototypes[i] = new DetailPrototype();
			DetailPrototypes[i].usePrototypeMesh = true;
            DetailPrototypes[i].prototype = GetRendererObject( Prefab );
			DetailPrototypes[i].useInstancing = true;
			DetailPrototypes[i].renderMode = DetailRenderMode.VertexLit;
        }

        AllTerrains[0].terrainData.detailPrototypes = DetailPrototypes;

        for( int i = 0; i < TIC.GrassTextures.Count; i++ )
        {
            string GrassTexturePath = "Assets/" + TIC.GrassTextures[i];
            Obj = AssetDatabase.LoadAssetAtPath<Texture>( GrassTexturePath );
            if( Obj == null )
                return;
            Tex = Obj as Texture2D;
            if( !Tex.isReadable )
            {
                Debug.LogError( "Tex " + Tex.ToString() + " is not readable !" );
                return;
            }
            Color32[] Pixels32 = Tex.GetPixels32();
			int[,] DetailMap = new int[Tex.width, Tex.height];
            for( int y = 0; y < Tex.height; y++ )
            {
                for( int x = 0; x < Tex.width; x++ )
				{
					if( Pixels32[y * Tex.width + x].r > 0 )
						DetailMap[x, y] = Pixels32[ y * Tex.width + x ].r/64;//similar to UE
					else
                        DetailMap[x, y] = 0;
                }
			}
			
            AllTerrains[0].terrainData.SetDetailLayer( 0, 0, i, DetailMap );
        }

    }
    [MenuItem( "UTU/SetEditorCameraToMainCamera" )]
    static void SetEditorCameraToMainCamera()
    {
        Camera cam = SceneView.lastActiveSceneView.camera;

        GameObject MainCamera = GameObject.Find("Main Camera");
        if( MainCamera != null )
        {
            MainCamera.transform.position = cam.gameObject.transform.position;
            MainCamera.transform.rotation = cam.gameObject.transform.rotation;
        }
    }
}
#endif