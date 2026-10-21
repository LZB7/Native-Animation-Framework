#pragma once

namespace Data
{
	typedef std::shared_ptr<std::vector<LinkableForm<RE::TESBoundObject>>> FurnitureList;

	class Furniture : public IdentifiableObject
	{
	public:
		static bool Parse(XMLUtil::Mapper& m, Furniture& out) {
			out.ParseID(m);
			out.sourceFile = std::filesystem::path(m.GetFileName()).lexically_normal().generic_string();

			std::string source;
			std::string form;
			std::string kw;
			m.GetArray([&](XMLUtil::Mapper& m) {
				source.clear();
				form.clear();
				kw.clear();
				bool hasOne = false;

				m(&source, ""s, true, false, "", "source");
				m(&form, ""s, true, false, "", "form");

				if (!source.empty() || !form.empty()) {
					out.forms.emplace_back(source, form);
					hasOne = true;
				}

				m(&kw, ""s, true, false, "", "keyword");

				if (!kw.empty()) {
					out.keywords.emplace_back(kw);
					hasOne = true;
				}

				if (!hasOne) {
					m.CustomFail("Furniture node has no applicable attributes!");
				}

				return m;
			}, "furniture", "Furniture group has no 'furniture' nodes!");

			m(&out.startAnim, std::optional<std::string>(std::nullopt), true, false, "", "startAnimation");
			m(&out.stopAnim, std::optional<std::string>(std::nullopt), true, false, "", "stopAnimation");

			return m;
		}

		void MergeFrom(Furniture&& other)
		{
			forms.insert(forms.end(), std::make_move_iterator(other.forms.begin()), std::make_move_iterator(other.forms.end()));
			keywords.insert(keywords.end(), std::make_move_iterator(other.keywords.begin()), std::make_move_iterator(other.keywords.end()));
			contributorCount += other.contributorCount;

			const bool otherWins = other.loadPriority > loadPriority ||
				(other.loadPriority == loadPriority && other.sourceFile < sourceFile);
			if (otherWins) {
				loadPriority = other.loadPriority;
				startAnim = std::move(other.startAnim);
				stopAnim = std::move(other.stopAnim);
				sourceFile = std::move(other.sourceFile);
			}
		}

		void FinalizeEntries()
		{
			// Parallel XML parsing makes arrival order nondeterministic. Normalize and
			// deduplicate textual identities before game-data linking so cold/cache/
			// reload paths all produce the same contribution set.
			std::ranges::sort(forms, {}, [](const auto& entry) { return entry.RawIdentity(); });
			std::vector<LinkableForm<RE::TESBoundObject>> uniqueForms;
			std::string previous;
			for (auto& entry : forms) {
				auto identity = entry.RawIdentity();
				if (uniqueForms.empty() || identity != previous) {
					previous = std::move(identity);
					uniqueForms.push_back(std::move(entry));
				}
			}
			forms.swap(uniqueForms);

			std::ranges::sort(keywords);
			keywords.erase(std::unique(keywords.begin(), keywords.end()), keywords.end());
		}

		void LinkAndDedupe(bool verbose)
		{
			// Textually different aliases may resolve to the same engine object. The
			// second pass deduplicates by pointer while preserving ordinary diagnostics
			// and dropping invalid explicit forms without discarding valid contributors.
			std::vector<LinkableForm<RE::TESBoundObject>> linkedForms;
			std::unordered_set<RE::TESBoundObject*> seenForms;
			for (auto& entry : forms) {
				if (auto* resolved = entry.get(verbose); resolved && seenForms.insert(resolved).second) linkedForms.push_back(std::move(entry));
			}
			forms.swap(linkedForms);

			std::vector<std::string> linkedKeywords;
			std::unordered_set<RE::BGSKeyword*> seenKeywords;
			std::unordered_set<std::string> seenUnresolved;
			for (auto& editorID : keywords) {
				if (auto* resolved = RE::TESForm::GetFormByEditorID<RE::BGSKeyword>(editorID); resolved) {
					if (seenKeywords.insert(resolved).second) linkedKeywords.push_back(std::move(editorID));
				} else if (seenUnresolved.insert(editorID).second) {
					linkedKeywords.push_back(std::move(editorID));
				}
			}
			keywords.swap(linkedKeywords);
		}

		std::vector<std::string> keywords;
		std::vector<LinkableForm<RE::TESBoundObject>> forms;
		std::optional<std::string> startAnim;
		std::optional<std::string> stopAnim;
		std::string sourceFile;
		std::size_t contributorCount{ 1 };
	};
}
