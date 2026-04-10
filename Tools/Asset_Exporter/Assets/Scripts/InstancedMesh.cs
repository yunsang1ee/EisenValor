using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class InstancedMesh : MonoBehaviour
{
    // Start is called before the first frame update

    public List<Vector3> Translations = new List<Vector3>();
    public List<Quaternion> Rotations = new List<Quaternion>();
    public List<Vector3> Scales = new List<Vector3>();

    Mesh TargetMesh;
    Matrix4x4[] matrices;
    MeshRenderer MR;
    void Start()
    {
        MeshFilter MF = GetComponent<MeshFilter>();
        MR = GetComponent<MeshRenderer>();
        TargetMesh = MF.sharedMesh;
        MR.enabled = false;

        matrices = new Matrix4x4[Translations.Count];
        for(int i=0; i< Translations.Count; i++ )
        {
            Quaternion quat = Quaternion.Euler(0,0,0);
            Vector3 scale = new Vector3(1,1,1);
            if( Rotations.Count > i )
                quat = Rotations[i];
            if( Scales.Count > i )
                scale = Scales[i];
            matrices[i] = Matrix4x4.TRS( Translations[i], quat, scale );
        }
    }

    // Update is called once per frame
    void Update()
    {
        if( !TargetMesh || MR.sharedMaterials.Length == 0 )
            return;

        for( int submesh = 0; submesh < TargetMesh.subMeshCount; submesh++ )
        {
            Material mat = MR.sharedMaterials[0];

            if ( MR.sharedMaterials.Length >= submesh )
                mat = MR.sharedMaterials[submesh];

            Graphics.DrawMeshInstanced( TargetMesh, 0, mat, matrices );
        }
    }
}
