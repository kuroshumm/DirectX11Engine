$path = "d:\DirectX11_Sample\DirextX11Engine\SkinnedModel.cpp"
$content = Get-Content $path -Raw -Encoding Default

$start_string = "			struct VertexWeight {"
$end_string = "for (UINT f = 0; f < mesh->mNumFaces; ++f)"

$start_idx = $content.IndexOf($start_string)
if ($start_idx -eq -1) {
    Write-Host "Start string not found"
    exit 1
}

$end_idx = $content.IndexOf($end_string, $start_idx)
if ($end_idx -eq -1) {
    Write-Host "End string not found"
    exit 1
}

$replacement = @"
			std::vector<int> weightCount(vertices.size(),0);
			for (UINT boneIndex = 0, boneCount = mesh->mNumBones; boneIndex < boneCount; ++boneIndex)
			{
				aiBone* bone = mesh->mBones[boneIndex];
				for (UINT weightIndex = 0, weightNum = bone->mNumWeights; weightIndex < weightNum; ++weightIndex)
				{
					UINT vertexId = bone->mWeights[weightIndex].mVertexId;
					float weight = bone->mWeights[weightIndex].mWeight;

					int slot = weightCount[vertexId];

					if (slot < 4)
					{
						reinterpret_cast<UINT*>(&vertices[vertexId].BlendIndices)[slot] = CalcGlobalBoneIndex(bone);
						reinterpret_cast<float*>(&vertices[vertexId].BlendWeight)[slot] = weight;

						weightCount[vertexId]++;
					}
				}
			}

			// インデックスデータ取得
			
"@

$new_content = $content.Substring(0, $start_idx) + $replacement + $content.Substring($end_idx)
Set-Content $path $new_content -Encoding Default
Write-Host "Success"
