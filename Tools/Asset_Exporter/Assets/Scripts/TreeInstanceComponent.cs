using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEditor;

public class TreeInstanceComponent : MonoBehaviour
{
    // Start is called before the first frame update
    public List<Vector3> position = new List<Vector3>();
    public List<float> widthScale = new List<float>();
     
    public List<float> heightScale = new List<float>();

    public List<float> rotation = new List<float>();
    
    public List<GameObject> TreePrefabs = new List<GameObject>();

    //TerrainData
    public string HeightMapTexture = "";
    public float TerrainHeight = 0;

    //GrassData
    public List<string> GrassTextures = new List<string>();
    public List<GameObject> GrassPrefabs = new List<GameObject>();

    void Start()
    {
        
    }

    // Update is called once per frame
    void Update()
    {
        
    }
}