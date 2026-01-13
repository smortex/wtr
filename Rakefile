require "github_changelog_generator/task"

GitHubChangelogGenerator::RakeTask.new :changelog do |config|
  config.user = "smortex"
  config.project = "wtr"
  config.exclude_labels = %w[skip-changelog]
  config.future_release = File.read("CMakeLists.txt").match(/project\("wtr" VERSION "(?<version>[^"]+)"\)/)["version"]
end
