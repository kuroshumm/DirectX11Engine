$path = "d:\DirectX11_Sample\DirextX11Engine\SkinnedModel.cpp"
$content = Get-Content $path -Raw -Encoding Default

$replacement = @"
			struct VertexWeight {
				UINT boneIndex;
				float weight;
			};
			std::vector<std::vector<VertexWeight>> vertexWeights(vertices.size());

			for (UINT boneIndex = 0, boneCount = mesh->mNumBones; boneIndex < boneCount; ++boneIndex)
			{
				aiBone* bone = mesh->mBones[boneIndex];
				UINT globalBoneIndex = CalcGlobalBoneIndex(bone);

				for (UINT weightIndex = 0, weightNum = bone->mNumWeights; weightIndex < weightNum; ++weightIndex)
				{
					UINT vertexId = bone->mWeights[weightIndex].mVertexId;
					float weight = bone->mWeights[weightIndex].mWeight;
					vertexWeights[vertexId].push_back({ globalBoneIndex, weight });
				}
			}

			for (size_t v = 0; v < vertices.size(); ++v)
			{
				auto& weights = vertexWeights[v];
				
				std::sort(weights.begin(), weights.end(), [](const VertexWeight& a, const VertexWeight& b) {
					return a.weight > b.weight;
				});

				float sum = 0.0f;
				for (int j = 0; j < 4 && j < weights.size(); ++j)
				{
					reinterpret_cast<UINT*>(&vertices[v].BlendIndices)[j] = weights[j].boneIndex;
					reinterpret_cast<float*>(&vertices[v].BlendWeight)[j] = weights[j].weight;
					sum += weights[j].weight;
				}

				if (sum > 0.0f)
				{
					for (int j = 0; j < 4; ++j)
					{
						reinterpret_cast<float*>(&vertices[v].BlendWeight)[j] /= sum;
					}
				}
				else
				{
					reinterpret_cast<UINT*>(&vertices[v].BlendIndices)[0] = 0;
					reinterpret_cast<float*>(&vertices[v].BlendWeight)[0] = 1.0f;
				}
			}
"@

$start_idx = $content.IndexOf("std::vector<int> weightCount(vertices.size(),0);")
if ($start_idx -eq -1) {
    Write-Host "Start string not found"
    exit 1
}

$end_string = "for (UINT f = 0; f < mesh->mNumFaces; ++f)"
$end_idx = $content.IndexOf($end_string, $start_idx)
if ($end_idx -eq -1) {
    Write-Host "End string not found"
    exit 1
}

$new_content = $content.Substring(0, $start_idx) + $replacement + "`r`n`t`t`t" + $content.Substring($end_idx)

if (-not $new_content.Contains("#include <algorithm>")) {
    $new_content = "#include <algorithm>`r`n" + $new_content
}

Set-Content $path $new_content -Encoding Default
Write-Host "Success"
