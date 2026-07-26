import codecs

path = 'd:\\DirectX11_Sample\\DirextX11Engine\\SkinnedModel.cpp'
with codecs.open(path, 'r', 'utf-8', errors='ignore') as f:
    lines = f.readlines()

new_lines = []
if '#include <algorithm>' not in ''.join(lines):
    new_lines.append('#include <algorithm>\n')

skip = False
for i, line in enumerate(lines):
    if 'std::vector<int> weightCount(vertices.size(),0);' in line:
        skip = True
        new_lines.append('''\t\t\tstruct VertexWeight {
\t\t\t\tUINT boneIndex;
\t\t\t\tfloat weight;
\t\t\t};
\t\t\tstd::vector<std::vector<VertexWeight>> vertexWeights(vertices.size());

\t\t\tfor (UINT boneIndex = 0, boneCount = mesh->mNumBones; boneIndex < boneCount; ++boneIndex)
\t\t\t{
\t\t\t\taiBone* bone = mesh->mBones[boneIndex];
\t\t\t\tUINT globalBoneIndex = CalcGlobalBoneIndex(bone);

\t\t\t\tfor (UINT weightIndex = 0, weightNum = bone->mNumWeights; weightIndex < weightNum; ++weightIndex)
\t\t\t\t{
\t\t\t\t\tUINT vertexId = bone->mWeights[weightIndex].mVertexId;
\t\t\t\t\tfloat weight = bone->mWeights[weightIndex].mWeight;
\t\t\t\t\tvertexWeights[vertexId].push_back({ globalBoneIndex, weight });
\t\t\t\t}
\t\t\t}

\t\t\tfor (size_t v = 0; v < vertices.size(); ++v)
\t\t\t{
\t\t\t\tauto& weights = vertexWeights[v];
\t\t\t\t
\t\t\t\tstd::sort(weights.begin(), weights.end(), [](const VertexWeight& a, const VertexWeight& b) {
\t\t\t\t\treturn a.weight > b.weight;
\t\t\t\t});

\t\t\t\tfloat sum = 0.0f;
\t\t\t\tfor (int j = 0; j < 4 && j < weights.size(); ++j)
\t\t\t\t{
\t\t\t\t\treinterpret_cast<UINT*>(&vertices[v].BlendIndices)[j] = weights[j].boneIndex;
\t\t\t\t\treinterpret_cast<float*>(&vertices[v].BlendWeight)[j] = weights[j].weight;
\t\t\t\t\tsum += weights[j].weight;
\t\t\t\t}

\t\t\t\tif (sum > 0.0f)
\t\t\t\t{
\t\t\t\t\tfor (int j = 0; j < 4; ++j)
\t\t\t\t\t{
\t\t\t\t\t\treinterpret_cast<float*>(&vertices[v].BlendWeight)[j] /= sum;
\t\t\t\t\t}
\t\t\t\t}
\t\t\t\telse
\t\t\t\t{
\t\t\t\t\treinterpret_cast<UINT*>(&vertices[v].BlendIndices)[0] = 0;
\t\t\t\t\treinterpret_cast<float*>(&vertices[v].BlendWeight)[0] = 1.0f;
\t\t\t\t}
\t\t\t}\n''')
        continue

    if skip:
        if 'for (UINT f = 0; f < mesh->mNumFaces; ++f)' in line:
            skip = False
            new_lines.append(line)
        continue
    
    new_lines.append(line)

with codecs.open(path, 'w', 'shift_jis') as f:
    f.writelines(new_lines)
